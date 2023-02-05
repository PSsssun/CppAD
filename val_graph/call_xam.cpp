// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
// SPDX-FileCopyrightText: Bradley M. Bell <bradbell@seanet.com>
// SPDX-FileContributor: 2023-23 Bradley M. Bell
# include <cppad/cppad.hpp>
# include "atomic_xam.hpp"
/*
{xrst_begin val_call_op_xam.cpp dev}
{xrst_spell
   xam
}

Function Value Operator Example
###############################

{xrst_toc_hidden
   val_graph/atomic_xam.hpp
}
atomic_xam
**********
This example uses :ref:`val_atomic_xam-name` .

Source Code
***********
{xrst_literal
   // BEGIN_C++
   // END_C++
}

{xrst_end val_call_op_xam.cpp}
*/
// BEGIN_C++
// call_xam
bool call_xam(void)
{  bool ok = true;
   //
   // tape_t, Vector, addr_t
   using CppAD::local::val_graph::tape_t;
   using CppAD::local::val_graph::Vector;
   using CppAD::local::val_graph::addr_t;
   //
   // atomic_xam
   val_atomic_xam atomic_xam;
   //
   // callarg, op_arg
   Vector<addr_t> callarg(4), op_arg(2);
   //
   // f
   tape_t<double> tape;
   size_t n_ind = 4;
   size_t index_of_nan = size_t ( tape.set_ind(n_ind) );
   ok &= index_of_nan == n_ind;
   //
   // n_res, dep_vec
   size_t n_res = 2;
   Vector<addr_t> dep_vec(n_res);
   //
   // atomic_index
   size_t atomic_index = atomic_xam.atomic_index();
   //
   // f_0(x) = x[0] + x[1]
   // f_1(x) = x[2] * x[3]
   for(addr_t i = 0; i < 4; ++i)
      callarg[i] = i;
   size_t call_id    = 0;
   size_t n_call_res = 2;
   addr_t res_index = tape.record_call_op(
      atomic_index, call_id, n_call_res, callarg
   );
   //
   // dep_vec
   dep_vec[0] = res_index + 0;
   dep_vec[1] = res_index + 1;
   //
   // set_dep
   tape.set_dep( dep_vec );
   //
   // x
   Vector<double> x(n_ind);
   for(size_t i = 0; i < n_ind; ++i)
      x[i] = 2.0 + double(n_ind - i);
   //
   // trace
   bool trace = false;
   //
   // val_vec
   Vector<double> val_vec( tape.n_val() );
   for(size_t i = 0; i < n_ind; ++i)
      val_vec[i] = x[i];
   size_t compare_false = 0;
   tape.eval(trace, compare_false, val_vec);
   ok &= compare_false == 0;
   //
   // ok
   ok &= tape.arg_vec().size() == 1 + 4 + 4;
   ok &= tape.con_vec().size() == 1;
   ok &= tape.op_vec().size()  == 2;
   ok &= tape.n_val()          == n_ind + 1 + 2;
   //
   // y
   Vector<double> y(n_res);
   dep_vec = tape.dep_vec();
   for(size_t i = 0; i < n_res; ++i)
      y[i] = val_vec[ dep_vec[i] ];
   //
   // ok
   ok &= y[0] == x[0] + x[1];
   ok &= y[1] == x[2] * x[3];
   //
   return ok;
}
// END_C++
