# ifndef  CPPAD_LOCAL_VAL_GRAPH_VAL2FUN_HPP
# define  CPPAD_LOCAL_VAL_GRAPH_VAL2FUN_HPP
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
// SPDX-FileCopyrightText: Bradley M. Bell <bradbell@seanet.com>
// SPDX-FileContributor: 2003-23 Bradley M. Bell
// --------------------------------------------------------------------------
/*
------------------------------------------------------------------------------
*/

# include <cppad/core/ad_fun.hpp>
# include <cppad/local/op_code_dyn.hpp>
# include <cppad/local/val_graph/tape.hpp>
# include <cppad/local/pod_vector.hpp>

namespace CppAD { // BEGIN_CPPAD_NAMESPACE

// BEGIN_PROTOTYPE
template <class Base, class RecBase>
void ADFun<Base, RecBase>::val2fun(
   const local::val_graph::tape_t<Base>& val_tape  ,
   const CppAD::vector<size_t>&          dyn_ind   ,
   const CppAD::vector<size_t>&          var_ind   )
// END_PROTOTYPE
{  //
   // vector
   using CppAD::vector;
   //
   // op_info_t
   typedef typename local::val_graph::tape_t<Base>::op_info_t op_info_t;
   //
   // op_base_t, op_enum_t
   using local::val_graph::op_base_t;
   using local::val_graph::op_enum_t;
   //
   // val_op_vec, val_arg_vec, val_con_vec, val_dep_vec
   const vector<op_info_t>& val_op_vec = val_tape.op_vec();
   const vector<addr_t>&    val_arg_vec = val_tape.arg_vec();
   const vector<Base>&      val_con_vec = val_tape.con_vec();
   const vector<addr_t>&    val_dep_vec = val_tape.dep_vec();
   //
   // nan
   Base nan = CppAD::numeric_limits<Base>::quiet_NaN();
   //
   // par_addr
   // a temporary parameter index
   addr_t par_addr;
   //
   // var_addr
   // a temporary variable index
   addr_t var_addr;
   //
   // tmp_addr
   // a parameter or variable temporary index
   addr_t tmp_addr;
   //
   // val_n_ind
   // size of the independent value vector
   size_t val_n_ind = val_tape.n_ind();
   //
   // dyn_n_ind
   // number of independent dynamc parameters
   size_t dyn_n_ind = dyn_ind.size();
   //
   // var_n_ind
   // number of independent varibles
   size_t var_n_ind = var_ind.size();
   //
   CPPAD_ASSERT_KNOWN( dyn_n_ind + var_n_ind == val_n_ind,
      "val2fun: The number of independent variables and dynamic parameters\n"
      "is not equal to the size of the independent value vector"
   );
   //
   // n_val
   size_t n_val = val_tape.n_val();
   //
   // val_ad_type
   // initialize this mapping and set its value of independent vector
   vector<ad_type_enum> val_ad_type(n_val);
   for(size_t i = 0; i < size_t(number_ad_type_enum); ++i)
      val_ad_type[i] = number_ad_type_enum; // invalid
   for(size_t i = 0; i < dyn_n_ind; ++i)
   {  CPPAD_ASSERT_KNOWN( dyn_ind[i] < val_n_ind,
         "val2fun: number of independent values is <= dyn_ind[i]"
      );
      CPPAD_ASSERT_KNOWN( val_ad_type[ dyn_ind[i] ] != number_ad_type_enum,
         "val2fun: dep_ind[i] == dep_ind[j] for some i and j"
      );
      val_ad_type[ dyn_ind[i] ] = dynamic_enum;
   }
   for(size_t i = 0; i < var_n_ind; ++i)
   {  CPPAD_ASSERT_KNOWN( var_ind[i] < val_n_ind,
         "val2fun: number of independent values is <= var_ind[i]"
      );
      CPPAD_ASSERT_KNOWN( val_ad_type[ var_ind[i] ] != number_ad_type_enum,
         "val2fun: var_ind[i] == dep_ind[j] for some i and j\n"
         "or var_ind[i] == var_ind[j] for some i and j"
      );
      val_ad_type[ var_ind[i] ] = variable_enum;
   }
   //
   // val2fun_index
   // mapping from value index to index in the AD function object.
   // The meaning of this index depends on its ad_type.
   vector<addr_t> val2fun_index;
   //
   // rec
   // start a functon recording
   local::recorder<Base> rec;
   CPPAD_ASSERT_UNKNOWN( rec.num_op_rec() == 0 );
   rec.set_num_dynamic_ind(dyn_n_ind);
   rec.set_abort_op_index(0);
   rec.set_record_compare(false);
   //
   // rec, parameter
   // initialize with the value nan at index zero
   const local::pod_vector_maybe<Base>& parameter( rec.all_par_vec());
   CPPAD_ASSERT_UNKNOWN( parameter.size() == 0 );
   par_addr = rec.put_con_par(nan);
   CPPAD_ASSERT_UNKNOWN( par_addr == 0 );
   CPPAD_ASSERT_UNKNOWN( isnan( parameter[par_addr] ) );
   //
   // rec
   // Place the variable with index 0 in the tape
   CPPAD_ASSERT_NARG_NRES(local::BeginOp, 1, 1);
   rec.PutOp(local::BeginOp);
   rec.PutArg(0); // parameter argumnet is the nan above
   //
   // rec, val2fun_index
   // put the independent value vector in the function recording
   for(size_t i = 0; i < val_n_ind; ++i)
   {  if( val_ad_type[i] == dynamic_enum )
      {  par_addr = rec.put_dyn_par(nan, local::ind_dyn);
         CPPAD_ASSERT_UNKNOWN( isnan( parameter[par_addr] ) );
         val2fun_index.push_back( par_addr );
      }
      else
      {  CPPAD_ASSERT_UNKNOWN( val_ad_type[i] == variable_enum );
         var_addr = rec.PutOp( local::InvOp );
         val2fun_index.push_back( var_addr );
      }
   }
   CPPAD_ASSERT_UNKNOWN( size_t(par_addr) == dyn_n_ind );
   CPPAD_ASSERT_UNKNOWN( size_t(var_addr) == var_n_ind );
   //
   // ind_taddr_
   // address of the independent variables on variable tape
   ind_taddr_.resize(var_n_ind);
   for(size_t i = 0; i < var_n_ind; ++i)
      ind_taddr_[i] = i + 1;
   //
   // arg_ad_type, arg;
   vector<ad_type_enum> arg_ad_type;
   vector<addr_t>       arg;
   //
   // op_index
   for(size_t op_index = 0; op_index < n_val; ++op_index)
   {  //
      // op_ptr, agr_index, res_index, n_arg, op_enum
      const op_info_t& op_info       = val_op_vec[op_index];
      op_base_t<Base>* op_ptr    = op_info.op_ptr;
      addr_t           arg_index = op_info.arg_index;
      addr_t           res_index = op_info.res_index;
      size_t           n_arg     = op_ptr->n_arg(arg_index, val_arg_vec);
      op_enum_t        op_enum   = op_ptr->op_enum();
      //
      // arg, arg_ad_type, val_ad_type
      arg.resize(n_arg);
      arg_ad_type.resize(n_arg);
      switch( op_enum )
      {
         // map_op
         case local::val_graph::map_op_enum:
         // 2DO: implement this case
         assert( false );
         break;
         //
         // con_op
         case local::val_graph::con_op_enum:
         arg_ad_type[0]         = constant_enum;
         val_ad_type[res_index] = constant_enum;
         break;
         //
         // other cases
         default:
         ad_type_enum res_ad_type = constant_enum;
         for(addr_t i = 0; i < addr_t(n_arg); ++i)
         {  arg_ad_type[i] = val_ad_type[ val_arg_vec[arg_index + i] ];
            arg[i]         = val2fun_index[ val_arg_vec[arg_index + 1] ];
            CPPAD_ASSERT_UNKNOWN( arg_ad_type[i] < number_ad_type_enum );
            res_ad_type = std::max(res_ad_type, arg_ad_type[i] );
         }
         break;
      }
      //
      // rec, val2fun_index
      switch( op_enum )
      {  //
         // con_op_enum
         case local::val_graph::con_op_enum:
         CPPAD_ASSERT_UNKNOWN( n_arg = 1 );
         {  const Base& constant = val_con_vec[arg_index];
            par_addr = rec.put_con_par(par_addr);
            CPPAD_ASSERT_UNKNOWN( parameter[par_addr] == constant );
            //
            val2fun_index[res_index] = par_addr;
         }
         break;
         //
         // add_op_enum
         case local::val_graph::add_op_enum:
         CPPAD_ASSERT_UNKNOWN( n_arg == 2 );
         if( val_ad_type[res_index] == dynamic_enum )
         {  tmp_addr = rec.put_dyn_par(nan, local::add_dyn, arg[0], arg[1]);
            CPPAD_ASSERT_UNKNOWN( isnan( parameter[tmp_addr] ) );
         }
         else if( val_ad_type[res_index] == variable_enum )
         {  tmp_addr = rec.PutOp(local::AddvvOp);
            rec.PutArg(arg[0], arg[1]);
         }
         else if( arg_ad_type[0] == variable_enum )
         {  CPPAD_ASSERT_UNKNOWN( arg_ad_type[1] < variable_enum );
            tmp_addr = rec.PutOp(local::AddpvOp);
            rec.PutArg(arg[0], arg[1]);
         }
         else
         {  CPPAD_ASSERT_UNKNOWN( arg_ad_type[1] == variable_enum );
            CPPAD_ASSERT_UNKNOWN( arg_ad_type[0] < variable_enum );
            tmp_addr = rec.PutOp(local::AddpvOp);
            rec.PutArg(arg[1], arg[0]);
         }
         val2fun_index[res_index] = tmp_addr;
         break;
         //
         // sub_op_enum
         case local::val_graph::sub_op_enum:
         CPPAD_ASSERT_UNKNOWN( n_arg == 2 );
         if( val_ad_type[res_index] == dynamic_enum )
         {  tmp_addr = rec.put_dyn_par(nan, local::add_dyn, arg[0], arg[1]);
            CPPAD_ASSERT_UNKNOWN( isnan( parameter[tmp_addr] ) );
         }
         else if( val_ad_type[res_index] == variable_enum )
         {  tmp_addr = rec.PutOp(local::SubvvOp);
            rec.PutArg(arg[0], arg[1]);
         }
         else if( arg_ad_type[0] == variable_enum )
         {  CPPAD_ASSERT_UNKNOWN( arg_ad_type[1] < variable_enum );
            tmp_addr = rec.PutOp(local::SubvpOp);
            rec.PutArg(arg[0], arg[1]);
         }
         else
         {  CPPAD_ASSERT_UNKNOWN( arg_ad_type[1] == variable_enum );
            tmp_addr = rec.PutOp(local::SubpvOp);
            rec.PutArg(arg[0], arg[1]);
         }
         val2fun_index[res_index] = tmp_addr;
         break;
         //
         default:
         CPPAD_ASSERT_KNOWN( op_enum > local::val_graph::number_op_enum,
            "op_enum is not yet implemented"
         );
         break;
      }
   }
   // rec
   rec.PutOp(local::EndOp);
   //
   // ----------------------------------------------------------------------
   // End recording, set private member data except for
   // ----------------------------------------------------------------------
   //
   // dep_taddr_
   // address of the dependent variables on variable tape
   dep_taddr_.resize( var_n_ind );
   for(size_t i = 0; i < var_n_ind; ++i)
   {  var_addr      = val_dep_vec[i];
      dep_taddr_[i] = val2fun_index[var_addr];
   }
   //
   // bool values in this object except check_for_nan_
   has_been_optimized_        = false;
   //
   // size_t values in this object
   compare_change_count_      = 1;
   compare_change_number_     = 0;
   compare_change_op_index_   = 0;
   num_order_taylor_          = 0;
   cap_order_taylor_          = 0;
   num_direction_taylor_      = 0;
   num_var_tape_              = rec.num_var_rec();
   //
   // taylor_
   taylor_.resize(0);
   //
   // cskip_op_
   cskip_op_.resize( rec.num_op_rec() );
   //
   // load_op2var_
   load_op2var_.resize( rec.num_var_load_rec() );
   //
   // play_
   // Now that each dependent variable has a place in the recording,
   // and there is a EndOp at the end of the record, we can transfer the
   // recording to the player and and erase the recording.
   play_.get_recording(rec, var_n_ind);
   //
   // ind_taddr_
   // Note that play_ has been set, we can use it to check operators
   CPPAD_ASSERT_UNKNOWN( var_n_ind < num_var_tape_);
   for(size_t j = 0; j < var_n_ind; j++)
   {  CPPAD_ASSERT_UNKNOWN( play_.GetOp(j+1) == local::InvOp );
      CPPAD_ASSERT_UNKNOWN( ind_taddr_[j] = j+1 );
   }
   //
   // for_jac_sparse_pack_, for_jac_sparse_set_
   for_jac_sparse_pack_.resize(0, 0);
   for_jac_sparse_set_.resize(0,0);
   //
   // resize subgraph_info_
   subgraph_info_.resize(
      ind_taddr_.size(),   // n_dep
      dep_taddr_.size(),   // n_ind
      play_.num_op_rec(),  // n_op
      play_.num_var_rec()  // n_var
   );
   //
   // set the function name
   function_name_ = "";
   //
   //
   return;
}

} // END_CPPAD_NAMESPACE
// --------------------------------------------------------------------------
# endif
