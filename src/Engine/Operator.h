/*
Copyright (c) 2009-2016-2018, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 5.]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."

*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************

*/

/*! \file Operator.h
 *
 *  A structure to represent an operator
 *  Contains the actual data, the (J,M) that indicates
 * how this operator transforms, the fermionSign which
 * indicates if this operator commutes or anticommutes
 * with operators of the same class on different sites, and
 * other properties.
 *
 */
#ifndef OPERATOR_H
#define OPERATOR_H
#include "Su2Related.h"
#include "CrsMatrix.h"
#include "Io/IoSelector.h"
#include "InputNg.h"
#include "InputCheck.h"
#include "CanonicalExpression.h"
#include "Io/IoSerializerStub.h"

namespace Dmrg {

// This is a structure, don't add member functions here!
template<typename StorageType_>
struct Operator {

	enum {CAN_BE_ZERO = false, MUST_BE_NONZERO = true};

	typedef PsimagLite::Vector<SizeType>::Type VectorSizeType;
	typedef StorageType_ StorageType;
	typedef typename StorageType::value_type value_type;
	typedef typename PsimagLite::Real<value_type>::Type RealType;
	typedef std::pair<SizeType,SizeType> PairType;
	typedef Su2Related Su2RelatedType;
	typedef PsimagLite::Matrix<value_type> DenseMatrixType;

	Operator() : fermionSign(1), angularFactor(1) {}

	Operator(const StorageType& data1,
	         int fermionSign1,
	         const PairType& jm1,
	         RealType angularFactor1,
	         const Su2RelatedType& su2Related1)
	    : data(data1),
	      fermionSign(fermionSign1),
	      jm(jm1),
	      angularFactor(angularFactor1),
	      su2Related(su2Related1)
	{}

	template<typename IoInputType, typename SomeModelType>
	Operator(IoInputType& io,
	         SomeModelType& model,
	         bool checkNonZero,
	         PsimagLite::String prefix)
	{
		/*PSIDOC Operator
		 \item[TSPOperator] [String] One of \{\verb!cooked!, \verb!raw!,
		 \verb!expression!\}, in order to indicate
		 how the operator will be specified.
		 \item[OperatorExpression=] A label containing an operator expression.
		 Specify only if \verb!TSPOperator! was set to \verb!expression!
		 \item[COOKED\_OPERATOR] [String] A label naming the operator. This is model
		 dependent and must be listed in the \verb!naturalOperator! function for
		 the indicated in \verb!Model! in this input file. Do not specify unless
		 \verb!TSPOperator!
		 was set to \verb!cooked!.
		 \item[COOKED\_EXTRA] [VectorInteger] The first number is the number of numbers
		 to follow. The other numbers are degrees of freedom for the cooked operator
		 mentioned, and are passed as arguments (in order)
		 to the \verb!naturalOperator! function for
		 the indicated in \verb!Model! in this input file. Do not specify unless
		 \verb!TSPOperator!
		 was set to \verb!cooked!.
		 \item[RAW\_MATRIX] [MatrixComplexOrRealType] The number of rows and
		 columns of this matrix, followed by the matrix in zig-zag format.
		 Do not specify unless
		 \verb!TSPOperator!
		 was set to \verb!raw!.
		 \item[FERMIONSIGN] [RealType] Either 1 or -1, indicating if this operator
		 commutes or anticommutes at \emph{different} sites. Do not specify if
		 \verb!TSPOperator!
		 was set to \verb!expression!.
		 \item[JMVALUES] [Integer*2] If not using $SU(2)$ symmetry this is \verb!0 0!.
		 Else it is the $2j$ and $j+m$ for this operator. Do not specify if
		 \verb!TSPOperator!
		 was set to \verb!expression!.
		 \item[AngularFactor] [RealType] If not using $SU(2)$ symmetry this is \verb!1!.
		 Else FIXME. Do not specify if
		 \verb!TSPOperator!
		 was set to \verb!expression!.
		 */
		PsimagLite::String s = "";
		io.readline(s, prefix + "TSPOperator=");

		if (s == "cooked") {
			io.readline(s, prefix + "COOKED_OPERATOR=");
			VectorSizeType v;
			io.read(v,prefix + "COOKED_EXTRA");
			if (v.size() != 2)
				throw PsimagLite::RuntimeError("COOKED_EXTRA must be followed 2 v0 v1\n");
			data = model.naturalOperator(s,v[0],v[1]).data;
		} else if (s == "raw") {
			DenseMatrixType m;
			io.read(m, prefix + "RAW_MATRIX");
			if (checkNonZero) checkNotZeroMatrix(m);
			fullMatrixToCrsMatrix(data,m);
			PsimagLite::String msg = "WARNING: RAW_MATRIX read, order of basis subject ";
			msg += "to change with DMRG++ version!\n";
			std::cerr<<msg;
			std::cout<<msg;
		} else if (s == "expression") {
			io.readline(s,prefix + "OperatorExpression=");
			int site = 0;
			typedef OperatorSpec<SomeModelType> OperatorSpecType;
			OperatorSpecType opSpec(model);
			PsimagLite::CanonicalExpression<OperatorSpecType> canonicalExpression(opSpec);
			Operator p = canonicalExpression(s, site);
			data = p.data;
			fermionSign = p.fermionSign;
			jm = p.jm;
			angularFactor = p.angularFactor;
			// TODO FIXME: deprecate cooked
			return;
		} else {
			PsimagLite::String str(__FILE__);
			str += " : " + ttos(__LINE__) + "\n";
			str += "TSPOperator= must be followed by one of";
			str += "raw, cooked, or expression, not " + s + "\n";
			throw PsimagLite::RuntimeError(str.c_str());
		}

		io.readline(fermionSign,prefix + "FERMIONSIGN=");

		jm.first = jm.second = 0;
		angularFactor = 1;
		if (!SomeModelType::MyBasis::useSu2Symmetry())
			return;

		VectorSizeType v;
		io.read(v, prefix + "JMVALUES");
		if (v.size() != 2)
			err("FATAL: JMVALUES is not a vector of 2 values\n");
		jm.first = v[0]; jm.second = v[1];

		io.readline(angularFactor,prefix + "AngularFactor=");

		// FIXME: su2related needs to be set properly for when SU(2) is running
	}

	template<typename SomeMemResolvType>
	SizeType memResolv(SomeMemResolvType& mres,
	                   SizeType,
	                   PsimagLite::String msg = "") const
	{
		PsimagLite::String str = msg;
		str += "Operator";

		const char* start = reinterpret_cast<const char *>(this);
		const char* end = reinterpret_cast<const char *>(&fermionSign);
		SizeType total = mres.memResolv(&data, end-start, str + " data");

		start = end;
		end = reinterpret_cast<const char *>(&jm);
		total += mres.memResolv(&fermionSign, end-start, str + " fermionSign");

		start = end;
		end = reinterpret_cast<const char *>(&angularFactor);
		total += mres.memResolv(&jm, end-start, str + " jm");

		start = end;
		end = reinterpret_cast<const char *>(&su2Related);
		total += mres.memResolv(&angularFactor, end-start, str + " angularFactor");

		total += mres.memResolv(&su2Related,
		                        sizeof(*this) - total,
		                        str + " su2Related");

		return total;
	}

	void dagger()
	{
		StorageType data2 = data;
		transposeConjugate(data,data2);
	}

	void write(PsimagLite::String label,
	           PsimagLite::IoSerializer& ioSerializer) const
	{
		ioSerializer.createGroup(label);

		data.write(label + "/data", ioSerializer);
		ioSerializer.write(label + "/fermionSign", fermionSign);
		ioSerializer.write(label + "/jm", jm);
		ioSerializer.write(label + "/angularFactor", angularFactor);
		// su2Related.write(label + "/su2Related", ioSerializer);
	}

	void read(PsimagLite::String label,
	          PsimagLite::IoSerializer& ioSerializer)
	{
		data.read(label + "/data", ioSerializer);
		ioSerializer.read(fermionSign, label + "/fermionSign");
		ioSerializer.read(jm, label + "/jm");
		ioSerializer.read(angularFactor, label + "/angularFactor");
		// su2Related.read(label + "/su2Related", ioSerializer);
	}

	void write(std::ostream& os) const
	{
		os<<"TSPOperator=raw\n";
		os<<"RAW_MATRIX\n";
		DenseMatrixType m;
		crsMatrixToFullMatrix(m,data);
		os<<m;
		os<<"FERMIONSIGN="<<fermionSign<<"\n";
		os<<"JMVALUES 2 "<<jm.first<<" "<<jm.second<<"\n";
		os<<"AngularFactor="<<angularFactor<<"\n";
	}

	void send(int root,int tag,PsimagLite::MPI::CommType mpiComm)
	{
		data.send(root,tag,mpiComm);
		PsimagLite::MPI::send(fermionSign,root,tag+1,mpiComm);
		PsimagLite::MPI::send(jm,root,tag+2,mpiComm);
		PsimagLite::MPI::send(angularFactor,root,tag+3,mpiComm);
		Dmrg::send(su2Related,root,tag+4,mpiComm);
	}

	void recv(int root,int tag,PsimagLite::MPI::CommType mpiComm)
	{
		data.recv(root,tag,mpiComm);
		PsimagLite::MPI::recv(fermionSign,root,tag+1,mpiComm);
		PsimagLite::MPI::recv(jm,root,tag+2,mpiComm);
		PsimagLite::MPI::recv(angularFactor,root,tag+3,mpiComm);
		Dmrg::recv(su2Related,root,tag+4,mpiComm);
	}

	Operator& operator*=(value_type x)
	{
		data *= x;
		return *this;
	}

	Operator& operator*=(const Operator& other)
	{
		int fSaved = fermionSign;
		fermionSign = other.fermionSign;
		if (metaDiff(other) > 0)
			err("operator+= failed for Operator: metas not equal\n");

		StorageType crs;
		multiply(crs, data, other.data);
		data = crs;

		fermionSign = fSaved * other.fermionSign;

		return *this;
	}

	Operator& operator+=(const Operator& other)
	{
		if (metaDiff(other) > 0)
			err("operator+= failed for Operator: metas not equal\n");
		data += other.data;
		return *this;
	}

	SizeType metaDiff(const Operator& op2) const
	{
		const Operator& op1 = *this;

		SizeType code = 0;
		PsimagLite::Vector<bool>::Type b(4, false);

		b[0] = (op1.fermionSign != op2.fermionSign);
		b[1] = (op1.angularFactor != op2.angularFactor);
		b[2] = (op1.jm != op2.jm);
		//b[3] = (op1.su2Related != op2.su2Related);

		SizeType orFactor = 0;
		for (SizeType i = 0; i < b.size(); ++i) {
			if (b[i]) code |= orFactor;
			orFactor <<= 1;
		}

		return code;
	}

	StorageType data;
	// does this operator commute or anticommute with others of the
	// same class on different sites
	int fermionSign;
	PairType  jm; // angular momentum of this operator
	RealType angularFactor;
	Su2RelatedType su2Related;

private:

	void checkNotZeroMatrix(const DenseMatrixType& m) const
	{
		RealType norma = norm2(m);
		RealType eps = 1e-6;
		if (norma>eps) return;

		PsimagLite::String s(__FILE__);
		s += " : " + ttos(__LINE__) + "\n";
		s += "RAW_MATRIX or COOKED_OPERATOR ";
		s += " is less than " + ttos(eps) + "\n";
		std::cerr<<"WARNING: "<<s;
	}
};

template<typename SparseMatrixType>
void bcast(Operator<SparseMatrixType>& op)
{
	PsimagLite::bcast(op.data);
	PsimagLite::MPI::bcast(op.fermionSign);
	PsimagLite::MPI::bcast(op.jm);
	PsimagLite::MPI::bcast(op.angularFactor);
	bcast(op.su2Related);
}

template<typename SparseMatrixType,
         template<typename,typename> class SomeVectorTemplate,
         typename SomeAllocator1Type,
         typename SomeAllocator2Type>
void fillOperator(SomeVectorTemplate<SparseMatrixType*,SomeAllocator1Type>& data,
                  SomeVectorTemplate<Operator<SparseMatrixType>,SomeAllocator2Type>& op)
{
	for (SizeType i=0;i<data.size();i++) {
		data[i] = &(op[i].data);
	}
}

template<typename SparseMatrixType>
std::istream& operator>>(std::istream& is,Operator<SparseMatrixType>& op)
{
	is>>op.data;
	is>>op.fermionSign;
	SizeType theNumber2 = 0;
	is>>theNumber2;
	is>>op.jm;
	is>>op.angularFactor;
	is>>op.su2Related;
	return is;
}

template<typename SparseMatrixType>
std::ostream& operator<<(std::ostream& os,const Operator<SparseMatrixType>& op)
{
	os<<op.data;
	os<<op.fermionSign<<"\n";
	os<<"2\n"<<op.jm.first<<" "<<op.jm.second<<"\n";
	os<<op.angularFactor<<"\n";
	os<<op.su2Related;
	return os;
}
} // namespace Dmrg

/*@}*/
#endif

