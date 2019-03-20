#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "src/common/base.h"
#include "src/common/error.h"
#include "src/common/statusor.h"
#include "src/stirling/info_class_manager.h"

/**
 * These are the steps to follow to add a new data source connector.
 * 1. If required, create a new SourceConnector class.
 * 2. Add a new Create function with the following signature:
 *    static std::unique_ptr<SourceConnector> Create().
 *    In this function create an InfoClassSchema (vector of InfoClassElement)
 * 3. Register the data source in the appropriate registry.
 */

namespace pl {
namespace stirling {

class InfoClassElement;
class InfoClassManager;

#define DUMMY_SOURCE_CONNECTOR(NAME)                                       \
  class NAME : public SourceConnector {                                    \
   public:                                                                 \
    static constexpr bool kAvailable = false;                              \
    static constexpr SourceType kSourceType = SourceType::kNotImplemented; \
    static constexpr char kName[] = "dummy";                               \
    inline static const DataElements kElements = {};                       \
    static std::unique_ptr<SourceConnector> Create(std::string name) {     \
      PL_UNUSED(name);                                                     \
      return nullptr;                                                      \
    }                                                                      \
  }

enum class SourceType : uint8_t {
  kEBPF = 1,
  kOpenTracing,
  kPrometheus,
  kFile,
  kUnknown,
  kNotImplemented
};

class SourceConnector : public NotCopyable {
 public:
  /**
   * @brief Defines whether the SourceConnector has an implementation.
   *
   * Default in the base class is true, and normally should not be changed in the derived class.
   *
   * However, a dervived class may want to redefine to false in certain special circumstances:
   * 1) a SourceConnector that is just a placeholder (not yet implemented).
   * 2) a SourceConnector that is not compilable (e.g. on Mac). See DUMMY_SOURCE_CONNECTOR macro.
   */
  static constexpr bool kAvailable = true;

  static constexpr std::chrono::milliseconds kDefaultSamplingPeriod{100};
  static constexpr std::chrono::milliseconds kDefaultPushPeriod{1000};

  SourceConnector() = delete;
  virtual ~SourceConnector() = default;

  Status Init() { return InitImpl(); }
  void TransferData(types::ColumnWrapperRecordBatch* record_batch) {
    return TransferDataImpl(record_batch);
  }
  Status Stop() { return StopImpl(); }

  SourceType type() const { return type_; }
  const std::string& source_name() const { return source_name_; }
  const DataElements& elements() const { return elements_; }

 protected:
  explicit SourceConnector(SourceType type, std::string source_name, DataElements elements)
      : elements_(std::move(elements)), type_(type), source_name_(std::move(source_name)) {}

  virtual Status InitImpl() = 0;
  virtual void TransferDataImpl(types::ColumnWrapperRecordBatch* record_batch) {
    PL_UNUSED(record_batch);
  }
  virtual Status StopImpl() = 0;
  /**
   * @brief Init Helper function: calculates monotonic clock to real time clock offset.
   *
   */
  void InitClockRealTimeOffset();
  /**
   * @brief If recording nsecs in your bt file, this function can be used to find the offset for
   * convert the result into realtime.
   */
  uint64_t ClockRealTimeOffset();

  DataElements elements_;
  uint64_t real_time_offset_;
  uint64_t kSecToNanosecFactor = 1000000000;

 private:
  SourceType type_;
  std::string source_name_;
};

}  // namespace stirling
}  // namespace pl
