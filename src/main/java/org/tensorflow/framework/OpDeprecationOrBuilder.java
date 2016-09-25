// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tensorflow/core/framework/op_def.proto

package org.tensorflow.framework;

public interface OpDeprecationOrBuilder extends
    // @@protoc_insertion_point(interface_extends:tensorflow.OpDeprecation)
    com.google.protobuf.MessageOrBuilder {

  /**
   * <pre>
   * First GraphDef version at which the op is disallowed.
   * </pre>
   *
   * <code>optional int32 version = 1;</code>
   */
  int getVersion();

  /**
   * <pre>
   * Explanation of why it was deprecated and what to use instead.
   * </pre>
   *
   * <code>optional string explanation = 2;</code>
   */
  java.lang.String getExplanation();
  /**
   * <pre>
   * Explanation of why it was deprecated and what to use instead.
   * </pre>
   *
   * <code>optional string explanation = 2;</code>
   */
  com.google.protobuf.ByteString
      getExplanationBytes();
}
