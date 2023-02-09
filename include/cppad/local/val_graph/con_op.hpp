# ifndef  CPPAD_LOCAL_VAL_GRAPH_CON_OP_HPP
# define  CPPAD_LOCAL_VAL_GRAPH_CON_OP_HPP
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
// SPDX-FileCopyrightText: Bradley M. Bell <bradbell@seanet.com>
// SPDX-FileContributor: 2023-23 Bradley M. Bell
// ----------------------------------------------------------------------------
# include <cppad/local/val_graph/base_op.hpp>

// define CPPAD_ASSERT_FIRST_CALL_NOT_PARALLEL
# include <cppad/utility/thread_alloc.hpp>

namespace CppAD { namespace local { namespace val_graph {
/*
{xrst_begin val_con_op dev}
{xrst_spell
   xam
}

The Constant Value Operator
###########################

Prototype
*********
{xrst_literal
   // BEGIN_CON_OP_T
   // END_CON_OP_T
}

Context
*******
The class is derived from :ref:`val_base_op-name` .
It overrides all its base class virtual member functions
and is a concrete class (it has no pure virtual functions).
This is not a unary operator because the operand is in the
constant vector and not the value vector.

get_instance
************
This static member function returns a pointer to a con_op_t object.

op_enum
*******
This override of :ref:`val_base_op@op_enum` returns ``con_op_enum`` .

n_aux
*****
This override of :ref:`val_base_op@n_aux` return 1.

n_arg
*****
This override of :ref:`val_base_op@n_arg` returns 1.

n_res
*****
This override of :ref:`val_base_op@n_res` returns 1.

eval
****
This override of :ref:`val_base_op@eval` sets
the result equal to
::

      con_vec[ arg_vec[ arg_index + 0 ] ]

trace
=====
If trace is true, this member function prints the following values:

.. csv-table::
   :widths: auto

   **meaning**,res_index,name,con_index,empty,res
   **width**,5,5,5,5,10

#. width is the number of characters used to print the value
   not counting an extra space that is placed between values
#. res_index is the value vector index for the result
#. name is the name of the operator
#. con_index is the constant vector index for the operand
#. empty is an empty string used to line up columns with binary operator
#. res is the result for this operator; i.e. value vector at index res_index

{xrst_toc_hidden
   val_graph/con_xam.cpp
}
Example
*******
The file :ref:`con_xam.cpp <val_con_xam.cpp-name>`
is an example and test that uses this operator.

{xrst_end val_con_op}
*/
// BEGIN_CON_OP_T
template <class Value>
class con_op_t : public op_base_t<Value> {
public:
   // get_instance
   static con_op_t* get_instance(void)
   {  CPPAD_ASSERT_FIRST_CALL_NOT_PARALLEL;
      static con_op_t instance;
      return &instance;
   }
   // op_enum
   op_enum_t op_enum(void) const override
   {  return con_op_enum; }
// END_CON_OP_T
   //
   // n_aux
   addr_t n_aux(void) const override
   {  return 1; }
   //
   // n_arg
   addr_t n_arg(
      addr_t                arg_index    ,
      const Vector<addr_t>& arg_vec      ) const override
   {  return 1; }
   //
   // n_res
   addr_t n_res(
      addr_t                arg_index    ,
      const Vector<addr_t>& arg_vec      ) const override
   {  return 1; }
   //
   // eval
   void eval(
      bool                  trace        ,
      addr_t                arg_index    ,
      const Vector<addr_t>& arg_vec      ,
      const Vector<Value>&  con_vec      ,
      addr_t                res_index    ,
      size_t&               compare_false,
      Vector<Value>&        val_vec      ) const override
   {  //
      // val_index, val_vec
      addr_t val_index    = arg_vec[ arg_index + 0 ];
      val_vec[res_index]  = con_vec[ val_index ];
      //
      // trace
      if( ! trace )
         return;
      //
      Value  res        = val_vec[res_index];
      std::printf(
         "%5d %5s %5d %5s %10.3g\n", res_index, "con", val_index, "", res
      );
   }
};

} } } // END_CPPAD_LOCAL_VAL_GRAPH_NAMESPACE

# endif
