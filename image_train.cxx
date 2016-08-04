/*!
 * Copyright (c) 2016 by Contributors
 */
#include <vector>
#include <string>
#include <cassert>

#include "include/symbol.h"
#include "include/optimizer.h"
#include "image_train.hpp"

using namespace mxnet::cpp;

ImageTrain::ImageTrain(int w, int h, int c) {
  width = w;
  height = h;
  channels = c;
  learning_rate = 1e-3;
  weight_decay = 1e-4;
  momentum = 0.9;
  clip_gradient = 10;
  is_built = false;
#ifdef GPU
  ctx_dev = Context(DeviceType::kGPU, 0);
#else
  ctx_dev = Context(DeviceType::kCPU, 0);
#endif
}

ImageTrain::~ImageTrain() {
  delete exec;
  delete opt;
}

void ImageTrain::setSeed(int seed) {
  MXRandomSeed(seed);
}

void ImageTrain::buildNet(int n, int b, char * n_name) {
  std::string net_name(n_name);
  assert(net_name == "inception_bn" ||
         net_name == "vgg" ||
         net_name == "lenet" ||
         net_name == "alexnet" ||
         net_name == "googlenet" ||
         net_name == "resnet");

  if (net_name == "inception_bn") {
    mxnet_sym = InceptionSymbol(n);
  } else if (net_name == "vgg") {
    mxnet_sym = VGGSymbol(n);
  } else if (net_name == "lenet") {
    mxnet_sym = LenetSymbol(n);
  } else if (net_name == "alexnet") {
    mxnet_sym = AlexnetSymbol(n);
  } else if (net_name == "googlenet") {
    mxnet_sym = GoogleNetSymbol(n);
  } else {
    mxnet_sym = ResNetSymbol(n);
  }
  setOptimizer(n, b);
}

void ImageTrain::setOptimizer(int n, int b) {
  batch_size = b;
  num_classes = n;

  preds.resize(num_classes * batch_size);

  opt = new Optimizer("ccsgd", learning_rate, weight_decay);
  opt->SetParam("momentum", momentum);
  opt->SetParam("rescale_grad", 1.0 / batch_size);
  opt->SetParam("clip_gradient", clip_gradient);

  args_map["data"] = NDArray(Shape(batch_size, channels, width, height), ctx_dev);
  args_map["data_label"] = NDArray(Shape(batch_size), ctx_dev);
  mxnet_sym.InferArgsMap(ctx_dev, &args_map, args_map);
  exec = mxnet_sym.SimpleBind(ctx_dev, args_map);
  is_built = true;
}

void ImageTrain::loadModel(char * model_path) {
  mxnet_sym = Symbol::Load(std::string(model_path));
  is_built = true;
}

const char * ImageTrain::toJson() {
  std::string tmp = mxnet_sym.ToJSON();
  return tmp.c_str();
}

void ImageTrain::saveModel(char * model_path) {
  if (!is_built) {
    std::cerr << "Network not built!" << std::endl;
  } else {
    mxnet_sym.Save(std::string(model_path));
  }
}

void ImageTrain::loadParam(char * param_path) {
  std::map<std::string, NDArray> parameters;
  NDArray::Load(std::string(param_path), nullptr, &parameters);
  for (const auto &k : parameters) {
    if (k.first.substr(0, 4) == "arg:") {
      auto name = k.first.substr(4, k.first.size() - 4);
      args_map[name] = k.second.Copy(ctx_dev);
    }
  }
  exec = mxnet_sym.SimpleBind(ctx_dev, args_map);
}

void ImageTrain::saveParam(char * param_path) {
  NDArray::Save(std::string(param_path), args_map);
  std::string param_path2 = std::string(param_path) + ".txt";
  std::ofstream myfile;
  myfile.open(param_path2.c_str());
  for (std::map<std::string, NDArray>::iterator iter = args_map.begin();
       iter != args_map.end(); iter++) {
    myfile << iter->first << " ";
    std::vector<mx_uint> shape_lst = iter->second.GetShape();
    size_t s = 1;
    for (size_t i = 0; i < shape_lst.size(); i++) {
      s = s * shape_lst[i];
    }
    std::vector<float> tmp(s);
    iter->second.SyncCopyToCPU(&tmp, s);
    for (size_t i = 0; i < s; i++) {
      myfile << tmp[i] << " ";
    }
    myfile << std::endl;
  }
  myfile.close();
}

std::vector<float> ImageTrain::train(float * data, float * label) {
  return execute(data, label, true);
}

std::vector<float> ImageTrain::predict(float * data, float * label) {
  std::cout << "This has been deprecated." << std::endl;
  return execute(data, label, false);
}

std::vector<float> ImageTrain::predict(float * data) {
  return execute(data, NULL, false);
}

std::vector<float> ImageTrain::execute(float * data, float * label, bool is_train) {
  if (!is_built) {
    std::cerr << "Network hasn't been built. "
        << "Please run buildNet() or loadModel() first." << std::endl;
    exit(0);
  }

  NDArray data_n = NDArray(data, Shape(batch_size, 3, width, height), ctx_dev);
  data_n.CopyTo(&args_map["data"]);

  if (is_train) {
    NDArray label_n = NDArray(label, Shape(batch_size), ctx_dev);
    label_n.CopyTo(&args_map["data_label"]);
  }

  NDArray::WaitAll();

  exec->Forward(is_train);
  // train or predict?
  if (is_train) {
    exec->Backward();
    exec->UpdateAll(opt, learning_rate, weight_decay);
  }

  NDArray::WaitAll();

  // get probs for prediction
  exec->outputs[0].SyncCopyToCPU(&preds, batch_size * num_classes);

  return preds;
}
