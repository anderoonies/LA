#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "compiler.h"

using namespace std;

int hash_id = 0;

string get_hash() {
  return "id_" + to_string(hash_id++);
}

int encode(int n) {
  return (n << 1) + 1;
};

int decode(int n) {
  return (n >> 1) - 1;
};

string get_free_var(string seed, shared_ptr<LA::Function> f) {
  string var = seed;
  bool valid;
  do {
    valid = true;
    for (auto bb : f->blocks)
      for (auto i : bb->instructions) {
        if (i->contains(var))
          valid = false;
      }
    if (!valid)
      var = "o" + var;
  } while (!valid);
  if (var[0] != '%')
    var = '%' + var;
  return var;
};

vector<string> get_free_vars(string seed, int n_vars, shared_ptr<LA::Function> f) {
  vector<string> vars;
  for (int i = 0; i < n_vars; i++) {
    string v_seed = seed + to_string(i);
    vars.push_back(get_free_var(v_seed, f));
  }
  return vars;
};

void add_declaration(shared_ptr<LA::Function> f, shared_ptr<LA::Declaration> dec, ofstream &output) {
  f->data_structs.insert(pair<string, shared_ptr<LA::Declaration>>(dec->var.name, dec));
  return;
};

void allocate_to_zero(shared_ptr<LA::Function> f, shared_ptr<LA::Declaration> dec, ofstream &output) {
  // to check for allocation
  if (dec->type.data_type == LA::array || dec->type.data_type == LA::tuple)
    output << dec->var.name << " <- 0\n";
  return;
};

int64_t get_dimensionality(shared_ptr<LA::Function> f, string variable_name) {
  return -1;
};

bool is_number(const std::string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

string safe_encode_constant(string in) {
  if (is_number(in))
    return to_string(encode(stoi(in)));
  else
    return in;
}

vector<string> decode_vars(shared_ptr<LA::Function> f, vector<LA::LA_item> vars, ofstream &output) {
  vector<string> replacements;
  for (auto v : vars) {
    string v_name = v.name;
    if (v_name.at(0) == '%')
      v_name.erase(0,1);
    string v_prime = get_free_var("prime" + v_name, f);
    replacements.push_back(v_prime);
    output << v_prime << " <- " << safe_encode_constant(v.name) << endl;
    output << v_prime << " <- " << v_prime << " >> 1\n";
  }
  return replacements;
};

void encode_vars(vector<string> vars, ofstream &output) {
  for (auto v : vars) {
    output << v << " <- " << v << " << 1\n";
    output << v << " <- " << v << " + 1\n";
  }
};

void Compiler::Compile(LA::Program p) {
  ofstream output;
  output.open("prog.IR");

  for (auto f : p.functions) {
    // write the function name and its args
    output << "define " << f->return_type.to_string() << " :" << f->name << "(";
    for (int var_i = 0; var_i < f->vars.size(); var_i++) {
      add_declaration(f, f->vars[var_i], output);
      output << f->vars[var_i]->type.to_string() << " " << f->vars[var_i]->var.name;
      if (var_i < f->vars.size() - 1)
        output << ", ";
    }
    output << "){\n";
    for (auto i : f->instructions) {
      // write the instructions one by one!
      if (shared_ptr<LA::Assignment> assn = dynamic_pointer_cast<LA::Assignment>(i))
      {
        output << assn->lhs.name << " <- " << safe_encode_constant(assn->rhs.name) << endl;
      }
      else if (shared_ptr<LA::Operation> op = dynamic_pointer_cast<LA::Operation>(i))
      {
        string op_string;
        switch(op->op) {
          case LA::plus:
            op_string = "+";
            break;
          case LA::minus:
            op_string = "-";
            break;
          case LA::times:
            op_string = "*";
            break;
          case LA::l3and:
            op_string = "&";
            break;
          case LA::lshift:
            op_string = "<<";
            break;
          case LA::rshift:
            op_string = ">>";
            break;
          case LA::lt:
            op_string = "<";
            break;
          case LA::lte:
            op_string = "<=";
            break;
          case LA::eq:
            op_string = "=";
            break;
          case LA::gt:
            op_string = ">";
            break;
          case LA::gte:
            op_string = ">=";
            break;
          default:
            break;
        }
        vector<LA::LA_item> vars_to_decode = op->toDecode();
        vector<string> replacements = decode_vars(f, vars_to_decode, output);
        shared_ptr<LA::Operation> decoded_op = op->decode(replacements);
        output << decoded_op->lhs.name << " <- ";
        output << safe_encode_constant(decoded_op->op_lhs.name) << " " << op_string << " " << safe_encode_constant(decoded_op->op_rhs.name) << endl;
        encode_vars({decoded_op->lhs.name}, output);
      }
      else if (shared_ptr<LA::Return> ret = dynamic_pointer_cast<LA::Return>(i))
      {
        output << "return" << endl;
      }
      else if (shared_ptr<LA::Declaration> dec = dynamic_pointer_cast<LA::Declaration>(i))
      {
        // TODO add the type to the function
        add_declaration(f, dec, output);
        allocate_to_zero(f, dec, output);
        output << dec->type.type_string << " " << dec->var.name << endl;
      }
      else if (shared_ptr<LA::ReturnValue> retv = dynamic_pointer_cast<LA::ReturnValue>(i))
      {
        output << "return " << safe_encode_constant(retv->value.name) << endl;
      }
      else if (shared_ptr<LA::Label> label = dynamic_pointer_cast<LA::Label>(i))
      {
        output << label->label.name << endl;
      }
      else if (shared_ptr<LA::Branch> branch = dynamic_pointer_cast<LA::Branch>(i))
      {
        output << "br " << branch->dest.name << endl;
      }
      else if (shared_ptr<LA::CBranch> cbranch = dynamic_pointer_cast<LA::CBranch>(i))
      {
        vector<LA::LA_item> vars_to_decode = cbranch->toDecode();
        vector<string> replacements = decode_vars(f, vars_to_decode, output);
        shared_ptr<LA::CBranch> decoded_cbranch = cbranch->decode(replacements);
        output << "br " << safe_encode_constant(decoded_cbranch->condition.name);
        output << " " << decoded_cbranch->then_dest.name << " " << decoded_cbranch->else_dest.name << endl;
      }
      else if (shared_ptr<LA::Call> call = dynamic_pointer_cast<LA::Call>(i))
      {
        auto data_iter = f->data_structs.find(call->callee.name);
        bool is_code = false;
        if (data_iter != f->data_structs.end())
          if (data_iter->second->type.data_type == LA::code)
            is_code = true;
        if (call->callee.name[0] == '%' && !is_code)
          call->callee.name.erase(0,1);
        if (call->callee.name != "array-error" && call->callee.name != "print" && !is_code)
          call->callee.name = ':' + call->callee.name;
        output << "call " << call->callee.name << "(";
        for (int arg_i = 0; arg_i < call->args.size(); arg_i++) {
          output << safe_encode_constant(call->args[arg_i].name);
          if (arg_i < call->args.size() - 1)
            output << ", ";
        }
        output << ")\n";
      }
      else if (shared_ptr<LA::CallAssign> call = dynamic_pointer_cast<LA::CallAssign>(i))
      {
        auto data_iter = f->data_structs.find(call->callee.name);
        bool is_code = false;
        if (data_iter != f->data_structs.end())
          if (data_iter->second->type.data_type == LA::code)
            is_code = true;
        output << call->lhs.name << " <- ";
        if (call->callee.name[0] == '%' && !is_code)
          call->callee.name.erase(0,1);
        if (call->callee.name != "array-error" && call->callee.name != "print" && !is_code)
          call->callee.name = ':' + call->callee.name;
        output << "call " << call->callee.name << "(";
        for (int arg_i = 0; arg_i < call->args.size(); arg_i++) {
          output << safe_encode_constant(call->args[arg_i].name);
          if (arg_i < call->args.size() - 1)
            output << ", ";
        }
        output << ")\n";
      }
      else if (shared_ptr<LA::TupleAllocate> alloc = dynamic_pointer_cast<LA::TupleAllocate>(i))
      {
        output << alloc->lhs.name << " <- new Tuple(" << safe_encode_constant(alloc->dimension.name) << ")\n";
      }
      else if (shared_ptr<LA::ArrayAllocate> alloc = dynamic_pointer_cast<LA::ArrayAllocate>(i))
      {
        output << alloc->lhs.name << " <- " << "new Array(";
        for (int i = 0; i < alloc->dimensions.size(); i++) {
          output << safe_encode_constant(alloc->dimensions.at(i).name);
          if (i < alloc->dimensions.size() - 1)
            output << ", ";
        }
        output << ")\n";

      }
      else if (shared_ptr<LA::IndexWrite> write = dynamic_pointer_cast<LA::IndexWrite>(i))
      {
        // checking allocation
        bool isArray = false;
        auto data_iter = f->data_structs.find(write->lhs.name);
        if (data_iter->second->type.data_type == LA::array)
          isArray = true;
        string isAllocated = get_free_var("isAllocated", f);
        string op_hash = get_hash();
        string abort = ":abort" + op_hash;
        string success = ":success" + op_hash;
        output << isAllocated << " <- " << write->lhs.name << " = 0\n";
        output << "br " << isAllocated << " " << abort << " " << success << endl;
        output << abort << endl << "call array-error(0, 0)" << endl << success << endl;
        vector<LA::LA_item> vars_to_decode = write->toDecode();
        vector<string> replacements = decode_vars(f, vars_to_decode, output);
        shared_ptr<LA::IndexWrite> decoded_write = write->decode(replacements);
        if (isArray) {
          // checking indexing
          // only necessary for arrays
          string out_of_bounds;
          string len_var;
          string encoded_index;
          for (int index_i = 0; index_i < write->indices.size(); index_i++) {
            // make vars
            out_of_bounds = ":out_of_bounds_" + to_string(index_i) + op_hash;
            success = ":success_" + to_string(index_i) + op_hash;
            encoded_index = "%encoded_" + to_string(index_i) + op_hash;
            // fetch the length of the dimension (as encoded) into len_var
            len_var = "%l_" + to_string(index_i) + op_hash;
            output << len_var << " <- length " << write->lhs.name << " " << index_i << endl;
            // encode the value of the index we're using
            output << encoded_index << " <- " << write->indices.at(index_i).name << endl;
            encode_vars({encoded_index}, output);
            // compare the length of the dimension to the index
            output << len_var << " <- " << encoded_index << " < " << len_var << endl;
            output << "br " << len_var << " " << success << " " << out_of_bounds << endl;
            output << out_of_bounds << "\ncall array-error(" << write->lhs.name << ", " << encoded_index << ")\n";
            output << success << endl;
          }
        }
        output << decoded_write->lhs.name;
        for (int index_i = 0; index_i < decoded_write->indices.size(); index_i++) {
          output << "[" << decoded_write->indices.at(index_i).name << "]";
        }
        output << " <- " << safe_encode_constant(decoded_write->rhs.name);
        output << endl;
      }
      else if (shared_ptr<LA::IndexRead> read = dynamic_pointer_cast<LA::IndexRead>(i))
      {
        bool isArray = false;
        auto data_iter = f->data_structs.find(read->rhs.name);
        if (data_iter->second->type.data_type == LA::array)
          isArray = true;
        // checking allocation
        string isAllocated = get_free_var("isAllocated", f);
        string op_hash = get_hash();
        string abort = ":abort" + op_hash;
        string success = ":success" + op_hash;
        output << isAllocated << " <- " << read->rhs.name << " = 0\n";
        output << "br " << isAllocated << " " << abort << " " << success << endl;
        output << abort << endl << "call array-error(0, 0)" << endl << success << endl;
        if (isArray) {
          // checking indexing
          string out_of_bounds;
          string len_var;
          string encoded_index;
          for (int index_i = 0; index_i < read->indices.size(); index_i++) {
            // make vars
            out_of_bounds = ":out_of_bounds_" + to_string(index_i) + op_hash;
            success = ":success_" + to_string(index_i) + op_hash;
            encoded_index = "%encoded_" + to_string(index_i) + op_hash;
            // fetch the length of the dimension (as encoded) into len_var
            len_var = "%l_" + to_string(index_i) + op_hash;
            output << len_var << " <- length " << read->rhs.name << " " << index_i << endl;
            // encode the value of the index we're using
            output << encoded_index << " <- " << read->indices.at(index_i).name << endl;
            encode_vars({encoded_index}, output);
            // compare the length of the dimension to the index
            output << len_var << " <- " << encoded_index << " < " << len_var << endl;
            output << "br " << len_var << " " << success << " " << out_of_bounds << endl;
            output << out_of_bounds << "\ncall array-error(" << read->rhs.name << ", " << encoded_index << ")\n";
            output << success << endl;
          }
        }
        vector<LA::LA_item> vars_to_decode = read->toDecode();
        vector<string> replacements = decode_vars(f, vars_to_decode, output);
        shared_ptr<LA::IndexRead> decoded_read = read->decode(replacements);
        output << decoded_read->lhs.name << " <- " << decoded_read->rhs.name;
        for (int index_i = 0; index_i < decoded_read->indices.size(); index_i++) {
          output << "[" << decoded_read->indices.at(index_i).name << "]";
        }
        output << endl;
      }
      else if (shared_ptr<LA::LengthRead> lr = dynamic_pointer_cast<LA::LengthRead>(i))
      {
        //checking allocation
        string isAllocated = get_free_var("isAllocated", f);
        string op_hash = get_hash();
        string abort = ":abort" + op_hash;
        string success = ":success" + op_hash;
        output << isAllocated << " <- " << read->rhs.name << " = 0\n";
        output << "br " << isAllocated << " " << success << " " << abort << endl;
        output << abort << endl << "call array-error(0, 0)" << endl << success << endl;
        vector<LA::LA_item> vars_to_decode = lr->toDecode();
        vector<string> replacements = decode_vars(f, vars_to_decode, output);
        shared_ptr<LA::LengthRead> decoded_lr = lr->decode(replacements);
        string v0 = get_free_var("v0", f);
        string v1 = get_free_var("v1", f);
        string v2 = get_free_var("v2", f);
        output << v0 << " <- " << safe_encode_constant(decoded_lr->index.name) << " * 8\n";
        output << v1 << " <- " << v0 << " + 16\n";
        output << v2 << " <- " << decoded_lr->rhs.name << " + " << v1 << endl;
        output << decoded_lr->lhs.name << " <- load " << v2 << endl;
      }
    }
    output << "}\n";
  }
  return;
};

LA::Program Compiler::Block(LA::Program p) {
  LA::Program newP;
  for (auto f : p.functions) {
    bool startBB = true;
    vector<shared_ptr<LA::Instruction>> newInsts;
    for (auto inst : f->instructions) {
      if (startBB) {
        shared_ptr<LA::Label> lbl = dynamic_pointer_cast<LA::Label>(inst);
        if (lbl == NULL) {
          string label_var = get_free_var("entry", f);
          shared_ptr<LA::Label> L = make_shared<LA::Label>();
          L->label.name = ':' + label_var.erase(0, 1);
          newInsts.push_back(L);
        }
        startBB = false;
      } else if (shared_ptr<LA::Label> lbl = dynamic_pointer_cast<LA::Label>(inst)) {
        shared_ptr<LA::Branch> g = make_shared<LA::Branch>();
        g->dest.name = lbl->label.name;
        newInsts.push_back(g);
      }
      newInsts.push_back(inst);
      if (inst->is_terminator()) {
        startBB = true;
      }
    }
    shared_ptr<LA::Function> newF = make_shared<LA::Function>();
    newF->return_type = f->return_type;
    newF->name = f->name;
    newF->vars = f->vars;
    newF->blocks = f->blocks;
    newF->instructions = newInsts;
    newF->data_structs = f->data_structs;
    newP.functions.push_back(newF);
  }
  return newP;
};
