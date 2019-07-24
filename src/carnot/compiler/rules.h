#pragma once
#include <memory>
#include <string>
#include <vector>

#include "src/carnot/compiler/compiler_state.h"
#include "src/carnot/compiler/ir_nodes.h"
#include "src/carnot/compiler/metadata_handler.h"
#include "src/carnot/compiler/pattern_match.h"
#include "src/carnot/compiler/registry_info.h"

namespace pl {
namespace carnot {
namespace compiler {
class Rule {
 public:
  Rule() = delete;
  virtual ~Rule() = default;

  explicit Rule(CompilerState* compiler_state) : compiler_state_(compiler_state) {}

  /**
   * @brief Executes the rule defined in apply.
   *
   * @param ir_graph : the graph to operate on.
   * @return true: if the rule changes the graph.
   * @return false: if the rule does nothing to the graph.
   * @return Status: error if something goes wrong during the rule application.
   */
  virtual StatusOr<bool> Execute(IR* ir_graph) const;

 protected:
  /**
   * @brief Applies the rule to a node.
   * Should include a check for type and should return true if it changes the node.
   * It can pass potential compiler errors through the Status.
   *
   * @param ir_node - the node to apply this rule to.
   * @return true: if the rule changes the node.
   * @return false: if the rule does nothing to the node.
   * @return Status: error if something goes wrong during the rule application.
   */

  virtual StatusOr<bool> Apply(IRNode* ir_node) const = 0;

  CompilerState* compiler_state_;
};

class DataTypeRule : public Rule {
  /**
   * @brief DataTypeRule evaluates the datatypes of any expressions that
   * don't have predetermined types.
   *
   * Currently resolves non-compile-time functions and column data types.
   *
   */

 public:
  explicit DataTypeRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;

 private:
  StatusOr<bool> EvaluateFunc(FuncIR* func) const;
  StatusOr<bool> EvaluateColumn(ColumnIR* column) const;
};

class SourceRelationRule : public Rule {
 public:
  explicit SourceRelationRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;

 private:
  StatusOr<bool> GetSourceRelation(OperatorIR* source_op) const;
  StatusOr<std::vector<std::string>> GetColumnNames(
      std::vector<ExpressionIR*> select_children) const;
  StatusOr<std::vector<ColumnIR*>> GetColumnsFromRelation(
      IRNode* node, std::vector<std::string> col_names,
      const table_store::schema::Relation& relation) const;
  StatusOr<table_store::schema::Relation> GetSelectRelation(
      IRNode* node, const table_store::schema::Relation& relation,
      const std::vector<std::string>& columns) const;
};

class OperatorRelationRule : public Rule {
  /**
   * @brief OperatorRelationRule sets up relations for non-source operators.
   */
 public:
  explicit OperatorRelationRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;

 private:
  StatusOr<bool> SetBlockingAgg(BlockingAggIR* agg_ir) const;
  StatusOr<bool> SetMap(MapIR* map_ir) const;
  StatusOr<bool> SetMetadataResolver(MetadataResolverIR* map_ir) const;
  StatusOr<bool> SetOther(OperatorIR* op) const;
};

class RangeArgExpressionRule : public Rule {
  /**
   * @brief CompilerExpressionRule handles the execution of compiler functions
   * for things like Range operator arguments.
   *
   * ie
   * df.Range(start = plc.now() - plc.minutes(2), end = plc.now())
   * should be evaluated to the integer values.
   * df.Range(start = 15191289803, end = 15191500000)
   *
   */
 public:
  explicit RangeArgExpressionRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
  StatusOr<IntIR*> EvalExpression(IRNode* ir_node) const;
  StatusOr<IntIR*> EvalFunc(std::string name, std::vector<IntIR*> evaled_args, FuncIR* func) const;
};

class VerifyFilterExpressionRule : public Rule {
  /**
   * @brief Quickly check to see whether filter expression returns True.
   *
   */
 public:
  explicit VerifyFilterExpressionRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
};

class ResolveMetadataRule : public Rule {
  /**
   * @brief Resolve metadata makes sure that metadata is properly added as a column in the table.
   * The job of this rule is to create, or map to a MetadataResolver for any MetadataIR in the
   * graph.
   */

 public:
  explicit ResolveMetadataRule(CompilerState* compiler_state, MetadataHandler* md_handler)
      : Rule(compiler_state), md_handler_(md_handler) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
  StatusOr<bool> HandleMetadata(MetadataIR* md_node) const;
  /** @brief Inserts a metadata resolver between the container op and parent op. */
  StatusOr<MetadataResolverIR*> InsertMetadataResolver(OperatorIR* container_op,
                                                       OperatorIR* parent_op) const;

 private:
  MetadataHandler* md_handler_;
};

class MetadataFunctionFormatRule : public Rule {
 public:
  explicit MetadataFunctionFormatRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
  StatusOr<MetadataLiteralIR*> WrapLiteral(DataIR* data, MetadataProperty* md_property) const;
};

class CheckMetadataColumnNamingRule : public Rule {
  /**
   * @brief Checks to make sure that the column naming scheme doesn't collide with metadata.
   *
   * Never modifies the graph, so should always return false.
   */
 public:
  explicit CheckMetadataColumnNamingRule(CompilerState* compiler_state, MetadataHandler* md_handler)
      : Rule(compiler_state), md_handler_(md_handler) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
  StatusOr<bool> CheckRelation(OperatorIR* op) const;

  MetadataHandler* md_handler_;
};

class MetadataResolverConversionRule : public Rule {
  /**
   * @brief Converts the metadata resolver into a Map after everything is done.
   */
 public:
  explicit MetadataResolverConversionRule(CompilerState* compiler_state) : Rule(compiler_state) {}

 protected:
  StatusOr<bool> Apply(IRNode* ir_node) const override;
  StatusOr<bool> ReplaceMetadataResolver(MetadataResolverIR* md_resolver) const;

  StatusOr<MapIR*> MakeMap(MetadataResolverIR* md_resolver) const;
  StatusOr<bool> SwapInMap(MetadataResolverIR* md_resolver, MapIR* map) const;
  StatusOr<std::string> FindKeyColumn(const table_store::schema::Relation& parent_relation,
                                      MetadataProperty* property, ColumnIR* col) const;
};
}  // namespace compiler
}  // namespace carnot
}  // namespace pl
