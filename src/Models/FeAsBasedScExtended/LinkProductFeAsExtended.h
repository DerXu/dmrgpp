/*
Copyright (c) 2009-2014, UT-Battelle, LLC
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
/** \ingroup DMRG */
/*@{*/

/*! \file LinkProductFeAsExtended.h
 *
 *  A class to represent product of operators that form a link or
 *  bond for this model
 *
 */
#ifndef LINK_PRODUCT_EX_H
#define LINK_PRODUCT_EX_H
#include "../Models/Heisenberg/LinkProductHeisenberg.h"
#include "../Models/FeAsModel/LinkProductFeAs.h"
#include "ProgramGlobals.h"

namespace Dmrg {

template<typename ModelHelperType, typename GeometryType>
class LinkProductFeAsExtended : public LinkProductBase<ModelHelperType, GeometryType> {

	typedef LinkProductBase<ModelHelperType, GeometryType> BaseType;
	typedef typename BaseType::AdditionalDataType AdditionalDataType;
	typedef typename BaseType::VectorSizeType VectorSizeType;
	typedef typename ModelHelperType::SparseMatrixType SparseMatrixType;
	typedef typename SparseMatrixType::value_type SparseElementType;
	typedef std::pair<SizeType,SizeType> PairType;
	typedef LinkProductFeAs<ModelHelperType, GeometryType> LinkProductFeAsType;
	typedef LinkProductHeisenberg<ModelHelperType, GeometryType> LinkProductHeisenbergType;
	typedef typename ModelHelperType::BasisType BasisType;
	typedef typename ModelHelperType::OperatorType OperatorType;

	enum {TERM_HOPPING, TERM_JPLUS, TERM_JZ};

	static const SizeType DEGREES_OF_FREEDOM = 4;

	static const bool ANISOTROPIC_IS_FALSE = false;

public:

	template<typename SomeInputType>
	LinkProductFeAsExtended(SomeInputType& io)
	    : BaseType(io, "Hopping SplusiSminuj SziSzj"),
	      lFeAs_(io),
	      lHeis_(io, ANISOTROPIC_IS_FALSE)
	{}

	SizeType dofs(SizeType term,const AdditionalDataType& additional) const
	{
		return (term == TERM_JPLUS || term == TERM_JZ) ?
		            lHeis_.dofs(term - 1,additional) :
		            lFeAs_.dofs(term - 1,additional);
	}

	// has only dependence on orbital
	void connectorDofs(VectorSizeType& edofs,
	                   SizeType term,
	                   SizeType dofs,
	                   const AdditionalDataType& additional) const
	{
		if (term==TERM_HOPPING)
			return lFeAs_.connectorDofs(edofs, term,dofs,additional);

		return lHeis_.connectorDofs(edofs, term - 1, dofs, additional);
	}

	void setLinkData(SizeType term,
	                 SizeType dofs,
	                 bool isSu2,
	                 ProgramGlobals::FermionOrBosonEnum& fermionOrBoson,
	                 PairType& ops,
	                 std::pair<char,char>& mods,
	                 SizeType& angularMomentum,
	                 typename BaseType::RealType& angularFactor,
	                 SizeType& category,const AdditionalDataType& additional) const
	{
		if (term==TERM_HOPPING)
			return lFeAs_.setLinkData(term,
			                          dofs,
			                          isSu2,
			                          fermionOrBoson,
			                          ops,
			                          mods,
			                          angularMomentum,
			                          angularFactor,
			                          category,
			                          additional);

		assert(term > 0);
		lHeis_.setLinkData(term - 1,
		                   dofs,
		                   isSu2,
		                   fermionOrBoson,
		                   ops,
		                   mods,
		                   angularMomentum,
		                   angularFactor,
		                   category,
		                   additional);

		SizeType offset1 = DEGREES_OF_FREEDOM;
		ops.first += offset1;
		ops.second += offset1;
	}

	void valueModifier(SparseElementType& value,
	                   SizeType term,
	                   SizeType dofs,
	                   bool isSu2,
	                   const AdditionalDataType& additional) const
	{
		if (term==TERM_HOPPING) return lFeAs_.valueModifier(value,
		                                                    term,
		                                                    dofs,
		                                                    isSu2,
		                                                    additional);

		assert(term > 0);
		lHeis_.valueModifier(value,term-1,dofs,isSu2,additional);
	}

	SizeType terms() const { return 3; }

private:

	LinkProductFeAsType lFeAs_;
	LinkProductHeisenbergType lHeis_;
}; // class LinkProductFeAsExtended
} // namespace Dmrg
/*@}*/
#endif //LINK_PRODUCT_EX_H
