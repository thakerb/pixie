#include <gtest/gtest.h>

#include "src/stirling/info_class_manager.h"
#include "src/stirling/seq_gen_connector.h"

namespace pl {
namespace stirling {

using types::DataType;

TEST(InfoClassElementTest, infoclass_element_proto_getters_test) {
  InfoClassElement element("user_percentage", DataType::FLOAT64);

  EXPECT_EQ("user_percentage", element.name());
  EXPECT_EQ(DataType::FLOAT64, element.type());

  stirlingpb::Element element_pb;
  element_pb = element.ToProto();
  EXPECT_EQ("user_percentage", element_pb.name());
  EXPECT_EQ(DataType::FLOAT64, element_pb.type());
}

TEST(InfoClassInfoSchemaTest, infoclass_mgr_proto_getters_test) {
  InfoClassSchema elements = {};
  InfoClassManager info_class_mgr("sequences_mgr");
  auto source = SeqGenConnector::Create("sequences");
  info_class_mgr.SetSourceConnector(source.get());
  ASSERT_OK(info_class_mgr.PopulateSchemaFromSource());

  EXPECT_EQ(SeqGenConnector::kElements.size(), info_class_mgr.Schema().size());
  EXPECT_EQ("sequences_mgr", info_class_mgr.name());

  stirlingpb::InfoClass info_class_pb;
  info_class_pb = info_class_mgr.ToProto();
  EXPECT_EQ(SeqGenConnector::kElements.size(), info_class_pb.elements_size());
  EXPECT_EQ("sequences_mgr", info_class_pb.name());
  EXPECT_EQ(0, info_class_pb.id());

  info_class_mgr.SetSubscription(true);
  stirlingpb::InfoClass subscribe_pb;
  subscribe_pb = info_class_mgr.ToProto();
  EXPECT_TRUE(subscribe_pb.subscribed());
}

}  // namespace stirling
}  // namespace pl
