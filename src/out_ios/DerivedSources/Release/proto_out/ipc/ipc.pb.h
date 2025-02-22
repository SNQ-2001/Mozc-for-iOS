// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ipc/ipc.proto

#ifndef PROTOBUF_ipc_2fipc_2eproto__INCLUDED
#define PROTOBUF_ipc_2fipc_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005001
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace mozc {
namespace ipc {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_ipc_2fipc_2eproto();
void protobuf_AssignDesc_ipc_2fipc_2eproto();
void protobuf_ShutdownFile_ipc_2fipc_2eproto();

class IPCPathInfo;

// ===================================================================

class IPCPathInfo : public ::google::protobuf::Message {
 public:
  IPCPathInfo();
  virtual ~IPCPathInfo();

  IPCPathInfo(const IPCPathInfo& from);

  inline IPCPathInfo& operator=(const IPCPathInfo& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const IPCPathInfo& default_instance();

  void Swap(IPCPathInfo* other);

  // implements Message ----------------------------------------------

  IPCPathInfo* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const IPCPathInfo& from);
  void MergeFrom(const IPCPathInfo& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional uint32 protocol_version = 4 [default = 0];
  inline bool has_protocol_version() const;
  inline void clear_protocol_version();
  static const int kProtocolVersionFieldNumber = 4;
  inline ::google::protobuf::uint32 protocol_version() const;
  inline void set_protocol_version(::google::protobuf::uint32 value);

  // optional string product_version = 5 [default = "0.0.0.0"];
  inline bool has_product_version() const;
  inline void clear_product_version();
  static const int kProductVersionFieldNumber = 5;
  inline const ::std::string& product_version() const;
  inline void set_product_version(const ::std::string& value);
  inline void set_product_version(const char* value);
  inline void set_product_version(const char* value, size_t size);
  inline ::std::string* mutable_product_version();
  inline ::std::string* release_product_version();
  inline void set_allocated_product_version(::std::string* product_version);

  // optional string key = 1;
  inline bool has_key() const;
  inline void clear_key();
  static const int kKeyFieldNumber = 1;
  inline const ::std::string& key() const;
  inline void set_key(const ::std::string& value);
  inline void set_key(const char* value);
  inline void set_key(const char* value, size_t size);
  inline ::std::string* mutable_key();
  inline ::std::string* release_key();
  inline void set_allocated_key(::std::string* key);

  // optional uint32 process_id = 2 [default = 0];
  inline bool has_process_id() const;
  inline void clear_process_id();
  static const int kProcessIdFieldNumber = 2;
  inline ::google::protobuf::uint32 process_id() const;
  inline void set_process_id(::google::protobuf::uint32 value);

  // optional uint32 thread_id = 3 [default = 0];
  inline bool has_thread_id() const;
  inline void clear_thread_id();
  static const int kThreadIdFieldNumber = 3;
  inline ::google::protobuf::uint32 thread_id() const;
  inline void set_thread_id(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:mozc.ipc.IPCPathInfo)
 private:
  inline void set_has_protocol_version();
  inline void clear_has_protocol_version();
  inline void set_has_product_version();
  inline void clear_has_product_version();
  inline void set_has_key();
  inline void clear_has_key();
  inline void set_has_process_id();
  inline void clear_has_process_id();
  inline void set_has_thread_id();
  inline void clear_has_thread_id();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::std::string* product_version_;
  static ::std::string* _default_product_version_;
  ::google::protobuf::uint32 protocol_version_;
  ::google::protobuf::uint32 process_id_;
  ::std::string* key_;
  ::google::protobuf::uint32 thread_id_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(5 + 31) / 32];

  friend void  protobuf_AddDesc_ipc_2fipc_2eproto();
  friend void protobuf_AssignDesc_ipc_2fipc_2eproto();
  friend void protobuf_ShutdownFile_ipc_2fipc_2eproto();

  void InitAsDefaultInstance();
  static IPCPathInfo* default_instance_;
};
// ===================================================================


// ===================================================================

// IPCPathInfo

// optional uint32 protocol_version = 4 [default = 0];
inline bool IPCPathInfo::has_protocol_version() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void IPCPathInfo::set_has_protocol_version() {
  _has_bits_[0] |= 0x00000001u;
}
inline void IPCPathInfo::clear_has_protocol_version() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void IPCPathInfo::clear_protocol_version() {
  protocol_version_ = 0u;
  clear_has_protocol_version();
}
inline ::google::protobuf::uint32 IPCPathInfo::protocol_version() const {
  return protocol_version_;
}
inline void IPCPathInfo::set_protocol_version(::google::protobuf::uint32 value) {
  set_has_protocol_version();
  protocol_version_ = value;
}

// optional string product_version = 5 [default = "0.0.0.0"];
inline bool IPCPathInfo::has_product_version() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void IPCPathInfo::set_has_product_version() {
  _has_bits_[0] |= 0x00000002u;
}
inline void IPCPathInfo::clear_has_product_version() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void IPCPathInfo::clear_product_version() {
  if (product_version_ != _default_product_version_) {
    product_version_->assign(*_default_product_version_);
  }
  clear_has_product_version();
}
inline const ::std::string& IPCPathInfo::product_version() const {
  return *product_version_;
}
inline void IPCPathInfo::set_product_version(const ::std::string& value) {
  set_has_product_version();
  if (product_version_ == _default_product_version_) {
    product_version_ = new ::std::string;
  }
  product_version_->assign(value);
}
inline void IPCPathInfo::set_product_version(const char* value) {
  set_has_product_version();
  if (product_version_ == _default_product_version_) {
    product_version_ = new ::std::string;
  }
  product_version_->assign(value);
}
inline void IPCPathInfo::set_product_version(const char* value, size_t size) {
  set_has_product_version();
  if (product_version_ == _default_product_version_) {
    product_version_ = new ::std::string;
  }
  product_version_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* IPCPathInfo::mutable_product_version() {
  set_has_product_version();
  if (product_version_ == _default_product_version_) {
    product_version_ = new ::std::string(*_default_product_version_);
  }
  return product_version_;
}
inline ::std::string* IPCPathInfo::release_product_version() {
  clear_has_product_version();
  if (product_version_ == _default_product_version_) {
    return NULL;
  } else {
    ::std::string* temp = product_version_;
    product_version_ = const_cast< ::std::string*>(_default_product_version_);
    return temp;
  }
}
inline void IPCPathInfo::set_allocated_product_version(::std::string* product_version) {
  if (product_version_ != _default_product_version_) {
    delete product_version_;
  }
  if (product_version) {
    set_has_product_version();
    product_version_ = product_version;
  } else {
    clear_has_product_version();
    product_version_ = const_cast< ::std::string*>(_default_product_version_);
  }
}

// optional string key = 1;
inline bool IPCPathInfo::has_key() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void IPCPathInfo::set_has_key() {
  _has_bits_[0] |= 0x00000004u;
}
inline void IPCPathInfo::clear_has_key() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void IPCPathInfo::clear_key() {
  if (key_ != &::google::protobuf::internal::kEmptyString) {
    key_->clear();
  }
  clear_has_key();
}
inline const ::std::string& IPCPathInfo::key() const {
  return *key_;
}
inline void IPCPathInfo::set_key(const ::std::string& value) {
  set_has_key();
  if (key_ == &::google::protobuf::internal::kEmptyString) {
    key_ = new ::std::string;
  }
  key_->assign(value);
}
inline void IPCPathInfo::set_key(const char* value) {
  set_has_key();
  if (key_ == &::google::protobuf::internal::kEmptyString) {
    key_ = new ::std::string;
  }
  key_->assign(value);
}
inline void IPCPathInfo::set_key(const char* value, size_t size) {
  set_has_key();
  if (key_ == &::google::protobuf::internal::kEmptyString) {
    key_ = new ::std::string;
  }
  key_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* IPCPathInfo::mutable_key() {
  set_has_key();
  if (key_ == &::google::protobuf::internal::kEmptyString) {
    key_ = new ::std::string;
  }
  return key_;
}
inline ::std::string* IPCPathInfo::release_key() {
  clear_has_key();
  if (key_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = key_;
    key_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}
inline void IPCPathInfo::set_allocated_key(::std::string* key) {
  if (key_ != &::google::protobuf::internal::kEmptyString) {
    delete key_;
  }
  if (key) {
    set_has_key();
    key_ = key;
  } else {
    clear_has_key();
    key_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  }
}

// optional uint32 process_id = 2 [default = 0];
inline bool IPCPathInfo::has_process_id() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void IPCPathInfo::set_has_process_id() {
  _has_bits_[0] |= 0x00000008u;
}
inline void IPCPathInfo::clear_has_process_id() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void IPCPathInfo::clear_process_id() {
  process_id_ = 0u;
  clear_has_process_id();
}
inline ::google::protobuf::uint32 IPCPathInfo::process_id() const {
  return process_id_;
}
inline void IPCPathInfo::set_process_id(::google::protobuf::uint32 value) {
  set_has_process_id();
  process_id_ = value;
}

// optional uint32 thread_id = 3 [default = 0];
inline bool IPCPathInfo::has_thread_id() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void IPCPathInfo::set_has_thread_id() {
  _has_bits_[0] |= 0x00000010u;
}
inline void IPCPathInfo::clear_has_thread_id() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void IPCPathInfo::clear_thread_id() {
  thread_id_ = 0u;
  clear_has_thread_id();
}
inline ::google::protobuf::uint32 IPCPathInfo::thread_id() const {
  return thread_id_;
}
inline void IPCPathInfo::set_thread_id(::google::protobuf::uint32 value) {
  set_has_thread_id();
  thread_id_ = value;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace ipc
}  // namespace mozc

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_ipc_2fipc_2eproto__INCLUDED
