/**********************************************************************************************************************
This file is part of the Control Toolbox (https://github.com/ethz-adrl/control-toolbox), copyright by ETH Zurich.
Licensed under the BSD-2 license (see LICENSE file in main directory)
**********************************************************************************************************************/

#pragma once

#include "CppADCodegenTrait.h"
#include "CppADDoubleTrait.h"
#include <iit/rbd/traits/TraitSelector.h>

namespace iit {
namespace rbd {
namespace tpl {

template <>
struct TraitSelector<CppAD::AD<double>>
{
    typedef ct::rbd::internal::CppADDoubleTrait Trait;
};

template <>
struct TraitSelector<CppAD::AD<CppAD::cg::CG<double>>>
{
    typedef ct::rbd::internal::CppADCodegenTrait Trait;
};


}  //namespace tpl
}  // namespace rbd
}  // namespace iit
