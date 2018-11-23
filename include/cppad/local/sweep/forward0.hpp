# ifndef CPPAD_LOCAL_SWEEP_FORWARD0_HPP
# define CPPAD_LOCAL_SWEEP_FORWARD0_HPP
/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under the terms of the
             Eclipse Public License Version 2.0.

This Source Code may also be made available under the following
Secondary License when the conditions for such availability set forth
in the Eclipse Public License, Version 2.0 are satisfied:
           GNU General Public License, Version 3.0.
---------------------------------------------------------------------------- */

# include <cppad/local/play/atom_op_info.hpp>

// BEGIN_CPPAD_LOCAL_SWEEP_NAMESPACE
namespace CppAD { namespace local { namespace sweep {
/*!
\file sweep/forward0.hpp
Compute zero order forward mode Taylor coefficients.
*/

/*
\def CPPAD_ATOMIC_CALL
This avoids warnings when NDEBUG is defined and user_ok is not used.
If NDEBUG is defined, this resolves to
\code
	atom_fun->forward
\endcode
otherwise, it respolves to
\code
	user_ok = atom_fun->forward
\endcode
This maco is undefined at the end of this file to facillitate is
use with a different definition in other files.
*/
# ifdef NDEBUG
# define CPPAD_ATOMIC_CALL atom_fun->forward
# else
# define CPPAD_ATOMIC_CALL user_ok = atom_fun->forward
# endif

/*!
\def CPPAD_FORWARD0_TRACE
This value is either zero or one.
Zero is the normal operational value.
If it is one, a trace of every forward0sweep computation is printed.
(Note that forward0sweep is not used if CPPAD_USE_FORWARD0 is zero).
*/
# define CPPAD_FORWARD0_TRACE 0

/*!
Compute zero order forward mode Taylor coefficients.

<!-- define forward0_doc_define -->
\tparam Base
The type used during the forward mode computations; i.e., the corresponding
recording of operations used the type AD<Base>.

\param s_out
Is the stream where output corresponding to PriOp operations will
be written.

\param print
If print is false,
suppress the output that is otherwise generated by the c PriOp instructions.

\param n
is the number of independent variables on the tape.

\param numvar
is the total number of variables on the tape.
This is also equal to the number of rows in the matrix taylor; i.e.,
play->num_var_rec().

\param play
The information stored in play
is a recording of the operations corresponding to the function
\f[
	F : {\bf R}^n \rightarrow {\bf R}^m
\f]
where \f$ n \f$ is the number of independent variables and
\f$ m \f$ is the number of dependent variables.

\param J
Is the number of columns in the coefficient matrix taylor.
This must be greater than or equal one.

<!-- end forward0_doc_define -->

\param taylor
\n
\b Input:
For i = 1 , ... , n,
<code>taylor [i * J + 0]</code>
variable with index j on the tape
(these are the independent variables).
\n
\n
\b Output:
For i = n + 1, ... , numvar - 1,
<code>taylor [i * J + 0]</code>
is the zero order Taylor coefficient for the variable with
index i on the tape.

\param cskip_op
Is a vector with size play->num_op_rec().
The input value of the elements does not matter.
Upon return, if cskip_op[i] is true, the operator index i
does not affect any of the dependent variable
(given the value of the independent variables).

\param var_by_load_op
Is a vector with size play->num_load_op_rec().
The input value of the elements does not matter.
Upon return,
it is the variable index corresponding the result for each load operator.
In the case where the index is zero,
the load operator results in a parameter (not a variable).
Note that the is no variable with index zero on the tape.

\param compare_change_count
Is the count value for changing number and op_index during
zero order foward mode.

\param compare_change_number
If compare_change_count is zero, this value is set to zero.
Otherwise, the return value is the number of comparision operations
that have a different result from when the information in
play was recorded.

\param compare_change_op_index
If compare_change_count is zero, this value is set to zero.
Otherwise it is the operator index (see forward_next) for the count-th
comparision operation that has a different result from when the information in
play was recorded.

\param not_used_rec_base
Specifies RecBase for this call.
*/

template <class Addr, class Base, class RecBase>
void forward0(
	const local::player<Base>* play,
	std::ostream&              s_out,
	bool                       print,
	size_t                     n,
	size_t                     numvar,
	size_t                     J,
	Base*                      taylor,
	bool*                      cskip_op,
	pod_vector<Addr>&          var_by_load_op,
	size_t                     compare_change_count,
	size_t&                    compare_change_number,
	size_t&                    compare_change_op_index,
	const RecBase&             not_used_rec_base
)
{	CPPAD_ASSERT_UNKNOWN( J >= 1 );
	CPPAD_ASSERT_UNKNOWN( play->num_var_rec() == numvar );

	// use p, q, r so other forward sweeps can use code defined here
	size_t p = 0;
	size_t q = 0;
	size_t r = 1;
	/*
	<!-- define forward0sweep_code_define -->
	*/

	// initialize the comparision operator counter
	if( p == 0 )
	{	compare_change_number   = 0;
		compare_change_op_index = 0;
	}

	// If this includes a zero calculation, initialize this information
	pod_vector<bool>   isvar_by_ind;
	pod_vector<size_t> index_by_ind;
	if( p == 0 )
	{	size_t i;

		// this includes order zero calculation, initialize vector indices
		size_t num = play->num_vec_ind_rec();
		if( num > 0 )
		{	isvar_by_ind.extend(num);
			index_by_ind.extend(num);
			for(i = 0; i < num; i++)
			{	index_by_ind[i] = play->GetVecInd(i);
				isvar_by_ind[i] = false;
			}
		}
		// includes zero order, so initialize conditional skip flags
		num = play->num_op_rec();
		for(i = 0; i < num; i++)
			cskip_op[i] = false;
	}

	// work space used by AFunOp.
	vector<bool> user_vx;        // empty vecotor
	vector<bool> user_vy;        // empty vecotor
	vector<Base> user_tx;        // argument vector Taylor coefficients
	vector<Base> user_ty;        // result vector Taylor coefficients
	//
	atomic_base<RecBase>* atom_fun = CPPAD_NULL; // user's atomic op
# ifndef NDEBUG
	bool                  user_ok   = false;      // atomic op return value
# endif
	//
	// information defined by forward_user
	size_t atom_old=0, atom_m=0, atom_n=0, atom_i=0, atom_j=0;
	enum_atom_state atom_state = start_atom; // proper initialization

	// length of the parameter vector (used by CppAD assert macros)
	const size_t num_par = play->num_par_rec();

	// pointer to the beginning of the parameter vector
	const Base* parameter = CPPAD_NULL;
	if( num_par > 0 )
		parameter = play->GetPar();

	// length of the text vector (used by CppAD assert macros)
	const size_t num_text = play->num_text_rec();

	// pointer to the beginning of the text vector
	const char* text = CPPAD_NULL;
	if( num_text > 0 )
		text = play->GetTxt(0);
	/*
	<!-- end forward0sweep_code_define -->
	*/

# if CPPAD_FORWARD0_TRACE
	// flag as to when to trace user function values
	bool user_trace            = false;

	// variable indices for results vector
	// (done differently for order zero).
	vector<size_t> atom_iy;
# endif

	// skip the BeginOp at the beginning of the recording
	play::const_sequential_iterator itr = play->begin();
	// op_info
	OpCode op;
	size_t i_var;
	const Addr*   arg;
	itr.op_info(op, arg, i_var);
	CPPAD_ASSERT_UNKNOWN( op == BeginOp );
	//
# if CPPAD_FORWARD0_TRACE
	std::cout << std::endl;
# endif
	bool flag; // a temporary flag to use in switch cases
	bool more_operators = true;
	while(more_operators)
	{
		// next op
		(++itr).op_info(op, arg, i_var);
		CPPAD_ASSERT_UNKNOWN( itr.op_index() < play->num_op_rec() );

		// check if we are skipping this operation
		while( cskip_op[itr.op_index()] )
		{	switch(op)
			{
				case AFunOp:
				{	// get information for this atomic function call
					CPPAD_ASSERT_UNKNOWN( atom_state == start_atom );
					play::atom_op_info<Base>(op, arg, atom_old, atom_m, atom_n);
					//
					// skip to the second AFunOp
					for(size_t i = 0; i < atom_m + atom_n + 1; ++i)
						++itr;
# ifndef NDEBUG
					itr.op_info(op, arg, i_var);
					CPPAD_ASSERT_UNKNOWN( op == AFunOp );
# endif
				}
				break;

				case CSkipOp:
				case CSumOp:
				itr.correct_before_increment();
				break;

				default:
				break;
			}
			(++itr).op_info(op, arg, i_var);
		}

		// action to take depends on the case
		switch( op )
		{
			case AbsOp:
			forward_abs_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case AddvvOp:
			forward_addvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case AddpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_addpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case AcosOp:
			// sqrt(1 - x * x), acos(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_acos_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AcoshOp:
			// sqrt(x * x - 1), acosh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_acosh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case AsinOp:
			// sqrt(1 - x * x), asin(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_asin_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AsinhOp:
			// sqrt(1 + x * x), asinh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_asinh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case AtanOp:
			// 1 + x * x, atan(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_atan_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AtanhOp:
			// 1 - x * x, atanh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_atanh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case CExpOp:
			// Use the general case with d == 0
			// (could create an optimzied verison for this case)
			forward_cond_op_0(
				i_var, arg, num_par, parameter, J, taylor
			);
			break;
			// ---------------------------------------------------

			case CosOp:
			// sin(x), cos(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_cos_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// ---------------------------------------------------

			case CoshOp:
			// sinh(x), cosh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_cosh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case CSkipOp:
			forward_cskip_op_0(
				i_var, arg, num_par, parameter, J, taylor, cskip_op
			);
			itr.correct_before_increment();
			break;
			// -------------------------------------------------

			case CSumOp:
			forward_csum_op(
				0, 0, i_var, arg, num_par, parameter, J, taylor
			);
			itr.correct_before_increment();
			break;
			// -------------------------------------------------

			case DisOp:
			forward_dis_op(p, q, r, i_var, arg, J, taylor);
			break;
			// -------------------------------------------------

			case DivvvOp:
			forward_divvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case DivpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_divpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case DivvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_divvp_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case EndOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 0);
			more_operators = false;
			break;
			// -------------------------------------------------

			case EqppOp:
			if( compare_change_count )
			{	forward_eqpp_op_0(
					compare_change_number, arg, parameter
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case EqpvOp:
			if( compare_change_count )
			{	forward_eqpv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case EqvvOp:
			if( compare_change_count )
			{	forward_eqvv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case ErfOp:
			forward_erf_op_0(i_var, arg, parameter, J, taylor);
			break;
# endif
			// -------------------------------------------------

			case ExpOp:
			forward_exp_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case Expm1Op:
			forward_expm1_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case InvOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			break;
			// ---------------------------------------------------

			case LdpOp:
			forward_load_p_op_0(
				play,
				i_var,
				arg,
				parameter,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data(),
				var_by_load_op.data()
			);
			break;
			// -------------------------------------------------

			case LdvOp:
			forward_load_v_op_0(
				play,
				i_var,
				arg,
				parameter,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data(),
				var_by_load_op.data()
			);
			break;
			// -------------------------------------------------

			case LeppOp:
			if( compare_change_count )
			{	forward_lepp_op_0(
					compare_change_number, arg, parameter
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------
			case LepvOp:
			if( compare_change_count )
			{	forward_lepv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case LevpOp:
			if( compare_change_count )
			{	forward_levp_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case LevvOp:
			if( compare_change_count )
			{	forward_levv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case LogOp:
			forward_log_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case Log1pOp:
			forward_log1p_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case LtppOp:
			if( compare_change_count )
			{	forward_ltpp_op_0(
					compare_change_number, arg, parameter
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------
			case LtpvOp:
			if( compare_change_count )
			{	forward_ltpv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case LtvpOp:
			if( compare_change_count )
			{	forward_ltvp_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case LtvvOp:
			if( compare_change_count )
			{	forward_ltvv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case MulpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_mulpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case MulvvOp:
			forward_mulvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case NeppOp:
			if( compare_change_count )
			{	forward_nepp_op_0(
					compare_change_number, arg, parameter
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case NepvOp:
			if( compare_change_count )
			{	forward_nepv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case NevvOp:
			if( compare_change_count )
			{	forward_nevv_op_0(
					compare_change_number, arg, parameter, J, taylor
				);
				{	if( compare_change_count == compare_change_number )
						compare_change_op_index = itr.op_index();
				}
			}
			break;
			// -------------------------------------------------

			case ParOp:
			forward_par_op_0(
				i_var, arg, num_par, parameter, J, taylor
			);
			break;
			// -------------------------------------------------

			case PowvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_powvp_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PowpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_powpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PowvvOp:
			forward_powvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PriOp:
			if( print ) forward_pri_0(s_out,
				arg, num_text, text, num_par, parameter, J, taylor
			);
			break;
			// -------------------------------------------------

			case SignOp:
			// cos(x), sin(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sign_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SinOp:
			// cos(x), sin(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sin_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SinhOp:
			// cosh(x), sinh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sinh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SqrtOp:
			forward_sqrt_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case StppOp:
			forward_store_pp_op_0(
				i_var,
				arg,
				num_par,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data()
			);
			break;
			// -------------------------------------------------

			case StpvOp:
			forward_store_pv_op_0(
				i_var,
				arg,
				num_par,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data()
			);
			break;
			// -------------------------------------------------

			case StvpOp:
			forward_store_vp_op_0(
				i_var,
				arg,
				num_par,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data()
			);
			break;
			// -------------------------------------------------

			case StvvOp:
			forward_store_vv_op_0(
				i_var,
				arg,
				num_par,
				J,
				taylor,
				isvar_by_ind.data(),
				index_by_ind.data()
			);
			break;
			// -------------------------------------------------

			case SubvvOp:
			forward_subvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case SubpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_subpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case SubvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_subvp_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case TanOp:
			// tan(x)^2, tan(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_tan_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case TanhOp:
			// tanh(x)^2, tanh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_tanh_op_0(i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case AFunOp:
			// start or end an atomic function call
			flag = atom_state == start_atom;
			atom_fun = play::atom_op_info<RecBase>(
				op, arg, atom_old, atom_m, atom_n
			);
			if( flag )
			{	atom_state = arg_atom;
				atom_i     = 0;
				atom_j     = 0;
				//
				user_tx.resize(atom_n);
				user_ty.resize(atom_m);
# if CPPAD_FORWARD0_TRACE
				atom_iy.resize(atom_m);
# endif
			}
			else
			{	atom_state = start_atom;
# ifndef NDEBUG
				if( ! user_ok )
				{	std::string msg =
						atom_fun->afun_name()
						+ ": atomic_base.forward: returned false";
					CPPAD_ASSERT_KNOWN(false, msg.c_str() );
				}
# endif
# if CPPAD_FORWARD0_TRACE
				user_trace = true;
# endif
			}
			break;

			case FunapOp:
			// parameter argument for a atomic function
			CPPAD_ASSERT_UNKNOWN( NumArg(op) == 1 );
			CPPAD_ASSERT_UNKNOWN( atom_state == arg_atom );
			CPPAD_ASSERT_UNKNOWN( atom_i == 0 );
			CPPAD_ASSERT_UNKNOWN( atom_j < atom_n );
			CPPAD_ASSERT_UNKNOWN( size_t( arg[0] ) < num_par );
			//
			user_tx[atom_j++] = parameter[ arg[0] ];
			//
			if( atom_j == atom_n )
			{	// call users function for this operation
				atom_fun->set_old(atom_old);
				CPPAD_ATOMIC_CALL(p, q,
					user_vx, user_vy, user_tx, user_ty
				);
				atom_state = ret_atom;
			}
			break;

			case FunavOp:
			// variable argument for a atomic function
			CPPAD_ASSERT_UNKNOWN( NumArg(op) == 1 );
			CPPAD_ASSERT_UNKNOWN( atom_state == arg_atom );
			CPPAD_ASSERT_UNKNOWN( atom_i == 0 );
			CPPAD_ASSERT_UNKNOWN( atom_j < atom_n );
			//
			user_tx[atom_j++] = taylor[ size_t(arg[0]) * J + 0 ];
			//
			if( atom_j == atom_n )
			{	// call users function for this operation
				atom_fun->set_old(atom_old);
				CPPAD_ATOMIC_CALL(p, q,
					user_vx, user_vy, user_tx, user_ty
				);
				atom_state = ret_atom;
			}
			break;

			case FunrpOp:
			// parameter result for a atomic function
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			CPPAD_ASSERT_UNKNOWN( atom_state == ret_atom );
			CPPAD_ASSERT_UNKNOWN( atom_i < atom_m );
			CPPAD_ASSERT_UNKNOWN( atom_j == atom_n );
			CPPAD_ASSERT_UNKNOWN( size_t( arg[0] ) < num_par );
# if CPPAD_FORWARD0_TRACE
			atom_iy[atom_i] = 0;
# endif
			atom_i++;
			if( atom_i == atom_m )
				atom_state = end_atom;
			break;

			case FunrvOp:
			// variable result for a atomic function
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			CPPAD_ASSERT_UNKNOWN( atom_state == ret_atom );
			CPPAD_ASSERT_UNKNOWN( atom_i < atom_m );
			CPPAD_ASSERT_UNKNOWN( atom_j == atom_n );
# if CPPAD_FORWARD0_TRACE
			atom_iy[atom_i] = i_var;
# endif
			taylor[ i_var * J + 0 ] = user_ty[atom_i++];
			if( atom_i == atom_m )
				atom_state = end_atom;
			break;
			// -------------------------------------------------

			case ZmulpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_zmulpv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case ZmulvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_zmulvp_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case ZmulvvOp:
			forward_zmulvv_op_0(i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			default:
			CPPAD_ASSERT_UNKNOWN(false);
		}
# if CPPAD_FORWARD0_TRACE
		size_t  d  = 0;
		if( user_trace )
		{	user_trace = false;

			CPPAD_ASSERT_UNKNOWN( op == AFunOp );
			CPPAD_ASSERT_UNKNOWN( NumArg(FunrvOp) == 0 );
			for(size_t i = 0; i < atom_m; i++) if( atom_iy[i] > 0 )
			{	size_t i_tmp   = (itr.op_index() + i) - atom_m;
				printOp(
					std::cout,
					play,
					i_tmp,
					atom_iy[i],
					FunrvOp,
					CPPAD_NULL
				);
				Base* Z_tmp = taylor + atom_iy[i] * J;
				printOpResult(
					std::cout,
					d + 1,
					Z_tmp,
					0,
					(Base *) CPPAD_NULL
				);
				std::cout << std::endl;
			}
		}
		Base*           Z_tmp   = taylor + i_var * J;
		if( op != FunrvOp )
		{
			printOp(
				std::cout,
				play,
				itr.op_index(),
				i_var,
				op,
				arg
			);
			if( NumRes(op) > 0 ) printOpResult(
				std::cout,
				d + 1,
				Z_tmp,
				0,
				(Base *) CPPAD_NULL
			);
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
# else
	}
# endif
	CPPAD_ASSERT_UNKNOWN( atom_state == start_atom );

	return;
}

} } } // END_CPPAD_LOCAL_SWEEP_NAMESPACE

// preprocessor symbols that are local to this file
# undef CPPAD_FORWARD0_TRACE
# undef CPPAD_ATOMIC_CALL

# endif
