// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
// SPDX-FileCopyrightText: Bradley M. Bell <bradbell@seanet.com>
// SPDX-FileContributor: 2023-23 Bradley M. Bell
# include "tape.hpp"

int main()
{  bool ok = true;
   //
   // f_0 (x) = x[0] + x[1]
   // f_1 (x) = (x[0] + x[1]) - x[1]
   //
   // f
   tape_t<double> tape;
   addr_t n_ind = 2;
   tape.set_ind(n_ind);
   //
   // op_arg
   Vector<addr_t> op_arg(2);
   op_arg[0] = 0; // x[0]
   op_arg[1] = 1; // x[1]
   //
   // res_arg
   Vector<addr_t> res_index(2);
   //
   // tape
   res_index[0] = tape.next_op(add_op_enum, op_arg);
   op_arg[0]    = res_index[0]; // x[0] + x[1]
   op_arg[1]    = 1;            // x[1]
   res_index[1] = tape.next_op(sub_op_enum, op_arg);
   //
   // x
   Vector<double> x(2);
   x[0] = 5.0;
   x[1] = 6.0;
   //
   // val_vec
   size_t n_val = size_t( tape.n_val() );
   Vector<double> val_vec(n_val);
   for(size_t i = 0; i < n_ind; ++i)
      val_vec[i] = x[i];
   //
   // val_vec
   tape.eval(val_vec);
   //
   // y
   Vector<double> y(2);
   for(size_t i = 0; i < 2; ++i)
      y[i] = val_vec[ res_index[i] ];
   //
   // ok
   ok &= y[0] == x[0] + x[1];
   ok &= y[1] == x[0];
   //
   std::cout << "x = " << x << "\n";
   std::cout << "y = " << y << "\n";
   if( ok )
   {  std::cout << "main: OK\n";
      return 0;
   }
   std::cout << "main: Error\n";
   return 1;
}
