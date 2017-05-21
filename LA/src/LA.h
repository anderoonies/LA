#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>

using namespace std;

namespace LA {
  struct LA_item {
    string name;
    bool operator==(const LA_item &other) const {
      return name == other.name;
    }
  };

  struct Variable : public LA_item {};

  struct LA_s : LA_item {};

  struct LA_t : LA_item {};

  struct LA_u : LA_item {};

  struct LA_callee : LA_item {
    bool runtime = false;
  };

  enum type { array, tuple, integer, code, LAvoid };

  struct Type {
    type data_type;
    int array_dim = 0;
    string type_string;

    string to_string() {
      string out_string;
      switch(data_type) {
        case array:
          out_string = "int64";
          for (int d = 0; d < array_dim; d++)
            out_string += "[]";
          break;
        case tuple:
          out_string = "tuple";
          break;
        case integer:
          out_string = "int64";
          break;
        case code:
          out_string = "code";
          break;
        case LAvoid:
          out_string = "void";
          break;
        default:
          out_string = "typeerror";
          break;
      }
      return out_string;
    }
  };

  enum Operator { plus, minus, times, l3and, lshift, rshift,
                    lt, lte, eq, gte, gt };

  struct Instruction {
    virtual ~Instruction() {};
    virtual bool contains(string v) {return false;};
    virtual vector<LA_item> toDecode() {return {};};
    virtual shared_ptr<Instruction> decode() {return nullptr;};
    virtual vector<LA_item> toEncode() {return {};};
    virtual shared_ptr<Instruction> encode() {return nullptr;};
    virtual bool is_terminator() { return false; };
  };

  struct Declaration : public Instruction {
    Variable var;
    Type type;

    bool contains(string v) {
      return var.name == v;
    };

    vector<LA_item> toDecode() {
      return {};
    };
  };

  struct ArrayAllocate : public Instruction {
    Variable lhs;
    vector<LA_t> dimensions;

    bool contains(string v) {
      for (auto dim : dimensions)
        if (dim.name == v)
          return true;
      return lhs.name == v;
    };
  };

  struct TupleAllocate : public Instruction {
    Variable lhs;
    LA_t dimension;

    bool contains(string v) {
      return (lhs.name == v || dimension.name == v);
    };
  };

  struct Assignment : public Instruction {
    Variable lhs;
    LA_s rhs;

    bool contains(string v) {
      return (lhs.name == v || rhs.name == v);
    };
  };

  struct Operation : public Instruction {
    Variable lhs;
    LA_t op_lhs;
    Operator op;
    LA_t op_rhs;

    bool contains(string v) {
      return (lhs.name == v ||
              op_lhs.name == v ||
              op_rhs.name == v);
    };

    vector<LA_item> toDecode() {
      return {static_cast<LA_item>(op_lhs), static_cast<LA_item>(op_rhs)};
    }

    shared_ptr<Operation> decode(vector<string> replacements) {
      shared_ptr<Operation> decoded = make_shared<Operation>();
      decoded->lhs = this->lhs;
      decoded->op = this->op;
      decoded->op_lhs = this->op_lhs;
      decoded->op_rhs = this->op_rhs;
      decoded->op_lhs.name = replacements.at(0);
      decoded->op_rhs.name = replacements.at(1);
      return decoded;
    };

    vector<LA_item> toEncode() {
      return {lhs};
    }

    shared_ptr<Operation> encode(vector<string> replacements) {
      shared_ptr<Operation> encoded = make_shared<Operation>();
      encoded->lhs = this->lhs;
      encoded->lhs.name = replacements.at(0);
      encoded->op = this->op;
      encoded->op_lhs = this->op_lhs;
      encoded->op_rhs = this->op_rhs;
      return encoded;
    }
  };

  struct IndexRead : public Instruction {
    Variable lhs;
    Variable rhs;
    vector<LA_t> indices;

    bool contains(string v) {
      for (auto i : indices)
        if (i.name == v)
          return true;
      return (lhs.name == v ||
              rhs.name == v);
    };

    vector<LA_item> toDecode() {
      vector<LA_item> ret_indices;
      for (auto i : this->indices) {
        ret_indices.push_back(static_cast<LA_item>(i));
      }
      return ret_indices;
    }

    shared_ptr<IndexRead> decode(vector<string> replacements) {
      shared_ptr<IndexRead> decoded = make_shared<IndexRead>();
      decoded->lhs = this->lhs;
      decoded->rhs = this->rhs;
      decoded->indices = this->indices;
      for (int i = 0; i < decoded->indices.size(); i++) {
        decoded->indices.at(i).name = replacements.at(i);
      }
      return decoded;
    }
  };

  struct IndexWrite : public Instruction {
    Variable lhs;
    LA_s rhs;
    vector<LA_t> indices;
    bool contains(string v) {
      for (auto i : indices)
        if (i.name == v)
          return true;
      return (lhs.name == v ||
              rhs.name == v);
    };

    vector<LA_item> toDecode() {
      vector<LA_item> ret_indices;
      for (auto i : this->indices) {
        ret_indices.push_back(static_cast<LA_item>(i));
      }
      return ret_indices;
    }

    shared_ptr<IndexWrite> decode(vector<string> replacements) {
      shared_ptr<IndexWrite> decoded = make_shared<IndexWrite>();
      decoded->lhs = this->lhs;
      decoded->rhs = this->rhs;
      decoded->indices = this->indices;
      for (int i = 0; i < decoded->indices.size(); i++) {
        decoded->indices.at(i).name = replacements.at(i);
      }
      return decoded;
    }
  };

  struct LengthRead : public Instruction {
    Variable lhs;
    Variable rhs;
    LA_t index;
    bool contains(string v) {
      return (lhs.name == v ||
              rhs.name == v ||
              index.name == v);
    };

    vector<LA_item> toDecode() {
      return {index};
    }

    shared_ptr<LengthRead> decode(vector<string> replacements) {
      shared_ptr<LengthRead> decoded = make_shared<LengthRead>();
      decoded->lhs = this->lhs;
      decoded->rhs = this->rhs;
      decoded->index = this->index;
      decoded->index.name = replacements.at(0);
      return decoded;
    }
  };

  struct Call : public Instruction {
    LA_callee callee;
    vector<LA_t> args;
    bool contains(string v) {
      for (auto a : args)
        if (a.name == v)
          return true;
      return (callee.name == v);
    };
  };

  struct CallAssign : public Instruction {
    Variable lhs;
    LA_callee callee;
    vector<LA_t> args;
  };

  struct Label : public Instruction {
    LA_item label;

    bool contains(string v) {
      return v == label.name;
    }
  };

  struct LA_te : public Instruction {
  };

  struct Branch : public LA_te {
    LA_item dest;
    bool is_terminator() { return true; };
  };

  struct CBranch : public LA_te {
    Variable condition;
    LA_item then_dest;
    LA_item else_dest;
    bool is_terminator() { return true; };

    bool contains(string v) {
      return condition.name == v;
    }

    vector<LA_item> toDecode() {
      return {condition};
    }

    shared_ptr<CBranch> decode(vector<string> replacements) {
      shared_ptr<CBranch> decoded = make_shared<CBranch>();
      decoded->then_dest = this->then_dest;
      decoded->else_dest = this->else_dest;
      decoded->condition = this->condition;
      decoded->condition.name = replacements.at(0);
      return decoded;
    }
  };

  struct Return : public LA_te {
    bool is_terminator() { return true; };
  };

  struct ReturnValue : public LA_te {
    LA_t value;
    bool is_terminator() { return true; };
  };

  struct BasicBlock {
    LA_item entry_point;
    LA_item exit_point;
    vector<shared_ptr<Instruction>> instructions;
  };

  struct Function {
    Type return_type;
    string name;
    vector<shared_ptr<Declaration>> vars;
    vector<shared_ptr<BasicBlock>> blocks;
    vector<shared_ptr<Instruction>> instructions;
    map<string, shared_ptr<Declaration>> data_structs;
  };

  struct Program {
    vector<shared_ptr<Function>> functions;
    map<string, shared_ptr<Instruction>> data_structs;
  };
}
