#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>

#include "LA.h"
#include "../../lib/PEGTL/pegtl.hh"
#include "../../lib/PEGTL/pegtl/analyze.hh"

using namespace pegtl;
using namespace std;

namespace LA {

  /*
   * Grammar rules from now on.
   */

  struct number :
    pegtl::seq<
      pegtl::opt<
        pegtl::sor<
          pegtl::one< '-' >,
          pegtl::one< '+' >
        >
      >,
      pegtl::plus<
        pegtl::digit
      >
    > {};

  struct comment :
    pegtl::disable<
      pegtl::one< ';' >,
      pegtl::until< pegtl::eolf >
    > {};

  struct seps :
    pegtl::star<
      pegtl::sor<
        pegtl::ascii::space,
        comment
      >
    > {};

  struct LA_arrow_rule :
    pegtl::seq<
      seps,
      pegtl::one< '<' >,
      pegtl::one< '-' >,
      seps
    > {};

  struct LA_name_rule :
    pegtl::seq<
      pegtl::plus<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >
        >
      >,
      pegtl::star<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >,
          pegtl::digit,
          pegtl::one< '-' >
        >
      >
    > {};

  struct LA_var_rule :
    pegtl::seq<
      pegtl::one< '%' >,
      LA_name_rule
    > {};

  struct LA_label_rule :
    pegtl::seq<
      pegtl::one< ':' >,
      LA_name_rule
    > {};

  struct LA_op_rule :
    pegtl::sor<
      pegtl::string< '<', '<' >,
      pegtl::string< '>', '>' >,
      pegtl::string< '<' >,
      pegtl::string< '<', '=' >,
      pegtl::string< '=' >,
      pegtl::string< '>', '=' >,
      pegtl::string< '>' >,
      pegtl::string< '+' >,
      pegtl::string< '-' >,
      pegtl::string< '*' >,
      pegtl::string< '&' >
    > {};

  struct LA_t_rule :
    pegtl::sor<
      LA_var_rule,
      number
    > {};

  struct LA_s_rule :
    pegtl::sor<
      LA_t_rule,
      LA_label_rule
    > {};

  struct LA_u_rule :
    pegtl::sor<
      LA_var_rule,
      LA_label_rule
    > {};

  struct LA_vars_rule :
    pegtl::sor<
      pegtl::seq<
        LA_t_rule,
        pegtl::star<
          pegtl::one< ',' >,
          seps,
          LA_t_rule
        >
      >,
      LA_t_rule,
      seps
    > {};

  struct LA_args_rule :
    pegtl::sor<
      pegtl::seq<
        LA_t_rule,
        pegtl::star<
          pegtl::one< ',' >,
          seps,
          LA_t_rule
        >
      >,
      LA_t_rule,
      seps
    > {};

  struct LA_callee_rule :
    pegtl::sor<
      LA_var_rule,
      LA_name_rule
    > {};

  struct LA_brackets_rule :
    pegtl::seq<
      pegtl::star<
        pegtl::string< '[',']'>
      >
    > {};

  struct LA_type_rule :
    pegtl::sor<
      pegtl::seq<
        pegtl::string< 'i','n','t','6','4' >,
        LA_brackets_rule
      >,
      pegtl::string< 't','u','p','l','e' >,
      pegtl::string< 'c','o','d','e' >
    > {};

  struct LA_T_rule :
    pegtl::sor<
      LA_type_rule,
      pegtl::string< 'v','o','i','d' >
    > {};

  struct LA_declaration_rule :
    pegtl::seq<
      seps,
      LA_type_rule,
      seps,
      LA_var_rule,
      seps
    > {};

  struct LA_declarations_rule :
    pegtl::sor<
      pegtl::seq<
        LA_declaration_rule,
        pegtl::star<
          pegtl::one< ',' >,
          seps,
          LA_declaration_rule
        >
      >,
      LA_declaration_rule,
      seps
    > {};

  struct LA_declaration_instruction_rule :
    pegtl::seq<
      seps,
      LA_declaration_rule,
      seps
    > {};

  struct LA_assignment_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      LA_s_rule,
      seps
    > {};

  struct LA_operation_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      LA_t_rule,
      seps,
      LA_op_rule,
      seps,
      LA_t_rule,
      seps
    > {};

  struct LA_indices_rule :
    pegtl::plus<
      seps,
      pegtl::one< '[' >,
      LA_t_rule,
      pegtl::one< ']' >,
      seps
    > {};

  struct LA_array_read_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      LA_var_rule,
      seps,
      LA_indices_rule,
      seps
    > {};

  struct LA_array_write_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_indices_rule,
      seps,
      LA_arrow_rule,
      seps,
      LA_s_rule,
      seps
    > {};

  struct LA_length_read_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      pegtl::string< 'l','e','n','g','t','h' >,
      seps,
      LA_var_rule,
      seps,
      LA_t_rule,
      seps
    > {};

  struct LA_branch_rule :
    pegtl::seq<
      seps,
      pegtl::string< 'b','r' >,
      seps,
      LA_label_rule,
      seps
    > {};

  struct LA_i_label_rule:
    pegtl::seq<
      seps,
      LA_label_rule,
      seps
    > {};

  struct LA_conditional_branch_rule :
    pegtl::seq<
      seps,
      pegtl::string< 'b','r' >,
      seps,
      LA_var_rule,
      seps,
      LA_label_rule,
      seps,
      LA_label_rule,
      seps
    > {};

  struct LA_return_rule :
    pegtl::seq<
      seps,
      pegtl::string< 'r','e','t','u','r','n' >,
      seps
    > {};

  struct LA_returnvalue_rule :
    pegtl::seq<
      seps,
      pegtl::string< 'r','e','t','u','r','n' >,
      seps,
      LA_t_rule,
      seps
    > {};

  struct LA_call_rule :
    pegtl::seq<
      seps,
      pegtl::string< 'c','a','l','l' >,
      seps,
      LA_callee_rule,
      seps,
      pegtl::one< '(' >,
      seps,
      LA_args_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > {};

  struct LA_call_assign_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      pegtl::string< 'c','a','l','l' >,
      seps,
      LA_callee_rule,
      seps,
      pegtl::one< '(' >,
      seps,
      LA_args_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > {};

  struct LA_array_allocate_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      pegtl::string< 'n','e','w' >,
      seps,
      pegtl::string< 'A','r','r','a','y','(' >,
      seps,
      LA_args_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > {};

  struct LA_tuple_allocate_rule :
    pegtl::seq<
      seps,
      LA_var_rule,
      seps,
      LA_arrow_rule,
      seps,
      pegtl::string< 'n','e','w' >,
      seps,
      pegtl::string< 'T','u','p','l','e','(' >,
      seps,
      LA_t_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > {};

  struct LA_instruction_rule :
    pegtl::sor<
      LA_array_read_rule,
      LA_array_write_rule,
      LA_length_read_rule,
      LA_array_allocate_rule,
      LA_tuple_allocate_rule,
      LA_operation_rule,
      LA_i_label_rule,
      LA_declaration_instruction_rule,
      LA_assignment_rule,
      LA_call_rule,
      LA_call_assign_rule,
      LA_branch_rule,
      LA_conditional_branch_rule,
      LA_returnvalue_rule,
      LA_return_rule
    > {};

  struct LA_instructions_rule :
    pegtl::star<
      seps,
      LA_instruction_rule,
      seps
    > {};

  struct LA_function_name_rule :
    pegtl::seq<
      seps,
      LA_name_rule,
      seps
    > {};

  struct LA_e_rule :
    pegtl::seq<
      seps,
      LA_label_rule,
      seps
    > {};

  struct LA_function_rule :
    pegtl::seq<
      seps,
      LA_T_rule,
      seps,
      LA_function_name_rule,
      seps,
      pegtl::one< '(' >,
      seps,
      LA_declarations_rule,
      seps,
      pegtl::one< ')' >,
      seps,
      pegtl::one< '{' >,
      seps,
      LA_instructions_rule,
      seps,
      pegtl::one< '}' >,
      seps
    > {};

  struct LA_program_rule :
    pegtl::star<
      LA_function_rule
    > {};

  struct entry_point_rule :
    LA_program_rule {};

  struct grammar :
    pegtl::must<
      entry_point_rule
    > {};

  /////////////
  //   DATA  //
  /////////////

  vector<LA::Function *> parsed_functions;
  vector<shared_ptr<LA::Variable>> parsed_variables;
  vector<LA::LA_s> parsed_s_vals;
  vector<LA::LA_t> parsed_t_vals;
  vector<LA::LA_t> parsed_indices;
  vector<LA::LA_u> parsed_u_vals;
  vector<LA::LA_t> parsed_args;
  vector<LA::Variable> parsed_vars;
  vector<shared_ptr<LA::Declaration>> parsed_declarations;
  LA_callee parsed_callee;
  vector<std::string> parsed_strings;
  vector<std::string> parsed_labels;
  vector<std::string> parsed_names;
  int64_t parsed_array_declaration_dimension = -1;
  Operator parsed_op;
  Type parsed_type;
  LA_item parsed_T;

  /////////////
  // ACTIONS //
  /////////////

  void add_instruction(LA::Program &p, shared_ptr<LA::Instruction> i) {
    p.functions.back()->instructions.push_back(i);
  };

  void clear_memory() {
    parsed_variables.clear();
    parsed_s_vals.clear();
    parsed_t_vals.clear();
    parsed_u_vals.clear();
    parsed_strings.clear();
    parsed_args.clear();
    parsed_indices.clear();
    parsed_array_declaration_dimension = -1;
    parsed_declarations.clear();
  }

  template< typename Rule >
    struct action : pegtl::nothing< Rule > {};

  template<> struct action < LA_name_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      parsed_names.push_back(in.string());
    }
  };

  template<> struct action < LA_label_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      parsed_labels.push_back(in.string());
    }
  };

  template<> struct action < LA_var_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Variable> var = make_shared<LA::Variable>();
      var->name = '%' + parsed_names.back();
      parsed_variables.push_back(var);
    }
  };

  template<> struct action < LA_op_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      std::string op = in.string();
      std::string plus ("+");
      std::string minus ("-");
      std::string times ("*");
      std::string l3and ("&");
      std::string lshift ("<<");
      std::string rshift (">>");
      std::string lt ("<");
      std::string lte ("<=");
      std::string eq ("=");
      std::string gte (">=");
      std::string gt (">");
      if (op.compare(plus) == 0)
        parsed_op = LA::plus;
      else if (op.compare(minus) == 0)
        parsed_op = LA::minus;
      else if (op.compare(times) == 0)
        parsed_op = LA::times;
      else if (op.compare(l3and) == 0)
        parsed_op = LA::l3and;
      else if (op.compare(lshift) == 0)
        parsed_op = LA::lshift;
      else if (op.compare(rshift) == 0)
        parsed_op = LA::rshift;
      else if (op.compare(lt) == 0)
        parsed_op = LA::lt;
      else if (op.compare(lte) == 0)
        parsed_op = LA::lte;
      else if (op.compare(eq) == 0)
        parsed_op = LA::eq;
      else if (op.compare(gte) == 0)
        parsed_op = LA::gte;
      else if (op.compare(gt) == 0)
        parsed_op = LA::gt;
    }
  };

  template<> struct action < LA_u_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::LA_u u;
      u.name = in.string();
      parsed_u_vals.push_back(u);
    }
  };

  template<> struct action < LA_t_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::LA_t t;
      t.name = in.string();
      parsed_t_vals.push_back(t);
    }
  };

  template<> struct action < LA_s_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::LA_s s;
      s.name = in.string();
      parsed_s_vals.push_back(s);
    }
  };

  template<> struct action < LA_args_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      // args are a property of a function call.
      parsed_args.insert(
          parsed_args.end(),
          parsed_t_vals.begin(),
          parsed_t_vals.end()
      );
    }
  };

  template<> struct action < LA_vars_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      // vars are a property of a function itself.
      // TODO

      //clear_memory();
    }
  };

  template<> struct action < LA_callee_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::LA_callee callee;
      callee.name = in.string();
      parsed_callee = callee;
    }
  };

  template<> struct action < LA_T_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      parsed_T.name = in.string();
    }
  };

  template<> struct action < LA_brackets_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      parsed_array_declaration_dimension = in.string().size() / 2;
    }
  };

  template<> struct action < LA_type_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      std::string type = in.string();
      std::string int_type = "int64";
      std::string tuple_type = "tuple";
      std::string code_type = "code";
      parsed_type.type_string = type;
      if (int_type.compare(type.substr(0, 5)) == 0) {
        if (parsed_array_declaration_dimension > 0) {
          parsed_type.data_type = LA::array;
          parsed_type.array_dim = parsed_array_declaration_dimension;
        } else {
          parsed_type.data_type = LA::integer;
        }
      } else if (tuple_type.compare(type) == 0) {
        parsed_type.data_type = LA::tuple;
      } else if (code_type.compare(type) == 0) {
        parsed_type.data_type = LA::code;
      }
    }
  };

  template<> struct action < LA_declaration_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Declaration> dec = make_shared<LA::Declaration>();
      dec->type = parsed_type;
      dec->var = *parsed_variables.back();
      parsed_declarations.push_back(dec);
    }
  };

  template<> struct action < LA_declarations_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      p.functions.back()->vars.insert(
        p.functions.back()->vars.end(),
        parsed_declarations.begin(),
        parsed_declarations.end()
      );
      clear_memory();
    }
  };

  template<> struct action < LA_declaration_instruction_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Declaration> dec = make_shared<LA::Declaration>();
      dec->type = parsed_type;
      dec->var = *parsed_variables.at(0);
      add_instruction(p, dec);
      clear_memory();
    }
  };

  template<> struct action < LA_assignment_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::Variable lhs = *parsed_variables.at(0);
      LA::LA_s rhs = parsed_s_vals.back();
      shared_ptr<LA::Assignment> assignment = make_shared<LA::Assignment>();
      assignment->lhs = lhs;
      assignment->rhs = rhs;
      add_instruction(p, assignment);
      clear_memory();
    }
  };

  template<> struct action < LA_operation_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      LA::Variable lhs = *parsed_variables.at(0);
      shared_ptr<LA::Operation> operation = make_shared<LA::Operation>();
      operation->lhs = lhs;
      operation->op_lhs = parsed_t_vals.end()[-2];
      operation->op_rhs = parsed_t_vals.end()[-1];
      operation->op = parsed_op;
      add_instruction(p, operation);
      clear_memory();
    }
  };

  template<> struct action < LA_branch_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Branch> branch = make_shared<LA::Branch>();
      LA::LA_item dest;
      dest.name = parsed_labels.back();
      branch->dest = dest;
      add_instruction(p, branch);
      clear_memory();
    }
  };

  template<> struct action < LA_conditional_branch_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::CBranch> cbranch = make_shared<LA::CBranch>();
      LA::LA_item then_dest;
      then_dest.name = parsed_labels.end()[-2];
      LA::LA_item else_dest;
      else_dest.name = parsed_labels.end()[-1];
      shared_ptr<LA::Variable> condition = parsed_variables.at(0);
      cbranch->condition = *condition;
      cbranch->then_dest = then_dest;
      cbranch->else_dest = else_dest;
      add_instruction(p, cbranch);
      clear_memory();
    }
  };

  template<> struct action < LA_i_label_rule >{
      static void apply( const pegtl::input &in, LA::Program &p){
        shared_ptr<LA::Label> label = make_shared<LA::Label>();
        LA::LA_item lbl;
        lbl.name = parsed_labels.back();
        label->label = lbl;
        add_instruction(p, label);
        clear_memory();
      }
  };

  template<> struct action < LA_call_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Call> call = make_shared<LA::Call>();
      call->callee = parsed_callee;
      call->args.insert(
          call->args.end(),
          parsed_args.begin(),
          parsed_args.end()
      );
      add_instruction(p, call);
      clear_memory();
    }
  };

  template<> struct action < LA_call_assign_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::CallAssign> calla = make_shared<LA::CallAssign>();
      calla->callee = parsed_callee;
      calla->args.insert(
          calla->args.end(),
          parsed_args.begin(),
          parsed_args.end()
      );
      calla->lhs = *parsed_variables.at(0);
      add_instruction(p, calla);
      clear_memory();
    }
  };

  template<> struct action < LA_array_allocate_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::ArrayAllocate> alloc = make_shared<LA::ArrayAllocate>();
      alloc->lhs = *parsed_variables.at(0);
      alloc->dimensions.insert(
          alloc->dimensions.end(),
          parsed_args.begin(),
          parsed_args.end()
      );
      add_instruction(p, alloc);
      clear_memory();
    }
  };

  template<> struct action < LA_tuple_allocate_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::TupleAllocate> alloc = make_shared<LA::TupleAllocate>();
      alloc->lhs = *parsed_variables.at(0);
      alloc->dimension = parsed_t_vals.back();
      add_instruction(p, alloc);
      clear_memory();
    }
  };

  template<> struct action < LA_length_read_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::LengthRead> lr = make_shared<LA::LengthRead>();
      lr->lhs = *parsed_variables.at(0);
      lr->rhs = *parsed_variables.back();
      lr->index = parsed_t_vals.back();
      add_instruction(p, lr);
      clear_memory();
    }
  };

  template<> struct action < LA_indices_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      parsed_indices.insert(
        parsed_indices.end(),
        parsed_t_vals.begin(),
        parsed_t_vals.end()
      );
    }
  };

  template<> struct action < LA_array_read_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::IndexRead> read = make_shared<LA::IndexRead>();
      read->lhs = *parsed_variables.at(0);
      read->rhs = *parsed_variables.at(1);
      read->indices.insert(
        read->indices.end(),
        parsed_indices.begin(),
        parsed_indices.end()
      );
      add_instruction(p, read);
      clear_memory();
    }
  };

  template<> struct action < LA_array_write_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::IndexWrite> write = make_shared<LA::IndexWrite>();
      write->lhs = *parsed_variables.at(0);
      write->rhs = parsed_s_vals.back();
      write->indices.insert(
        write->indices.end(),
        parsed_indices.begin(),
        parsed_indices.end()
      );
      add_instruction(p, write);
      clear_memory();
    }
  };

  template<> struct action < LA_return_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::Return> ret = make_shared<LA::Return>();
      add_instruction(p, ret);
      clear_memory();
    }
  };

  template<> struct action < LA_returnvalue_rule >{
    static void apply( const pegtl::input &in, LA::Program &p){
      shared_ptr<LA::ReturnValue> ret = make_shared<LA::ReturnValue>();
      LA::LA_t retvar = parsed_t_vals.back();
      ret->value = retvar;
      add_instruction(p, ret);
      clear_memory();
    }
  };

  template<> struct action < LA_function_name_rule >{
      static void apply( const pegtl::input &in, LA::Program &p){
        shared_ptr<LA::Function> fn = make_shared<LA::Function>();
        fn->name = parsed_names.back();
        if (parsed_T.name == "void") {
          fn->return_type.data_type = LA::LAvoid;
        } else {
          fn->return_type = parsed_type;
        }
        p.functions.push_back(fn);
        clear_memory();
      }
  };


  LA::Program LA_parse_file (char *fileName){

    /*
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< LA::grammar >();

    /*
     * Parse.
     */
    LA::Program p;
    pegtl::file_parser(fileName).parse< LA::grammar, LA::action >(p);

    return p;
  }
} // LA
