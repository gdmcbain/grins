//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// GRINS - General Reacting Incompressible Navier-Stokes
//
// Copyright (C) 2014-2015 Paul T. Bauman, Roy H. Stogner
// Copyright (C) 2010-2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-


// This class
#include "grins/low_mach_navier_stokes_base.h"

// GRINS
#include "grins/assembly_context.h"
#include "grins/constant_viscosity.h"
#include "grins/constant_specific_heat.h"
#include "grins/constant_conductivity.h"
#include "grins/grins_enums.h"
#include "grins/materials_parsing.h"

// libMesh
#include "libmesh/getpot.h"
#include "libmesh/string_to_enum.h"
#include "libmesh/fem_system.h"

namespace GRINS
{

  template<class Mu, class SH, class TC>
  LowMachNavierStokesBase<Mu,SH,TC>::LowMachNavierStokesBase(const std::string& physics_name,
                                                             const std::string& core_physics_name,
                                                             const GetPot& input)
    : Physics(physics_name, input),
      _flow_vars(input, core_physics_name),
      _temp_vars(input, core_physics_name),
      _p0_var(input, core_physics_name),
      _mu(input,MaterialsParsing::material_name(input,core_physics_name)),
      _cp(input,MaterialsParsing::material_name(input,core_physics_name)),
      _k(input,MaterialsParsing::material_name(input,core_physics_name))
  {
    this->read_input_options(input);

    return;
  }

  template<class Mu, class SH, class TC>
  LowMachNavierStokesBase<Mu,SH,TC>::~LowMachNavierStokesBase()
  {
    return;
  }

  template<class Mu, class SH, class TC>
  void LowMachNavierStokesBase<Mu,SH,TC>::read_input_options( const GetPot& input )
  {
    // Read thermodynamic state info
    MaterialsParsing::read_property( input,
                                     "Physics/"+low_mach_navier_stokes+"/p0",
                                     "ThermodynamicPressure",
                                     low_mach_navier_stokes,
                                     (*this),
                                     _p0 );

    MaterialsParsing::read_property( input,
                                     "Physics/"+low_mach_navier_stokes+"/T0",
                                     "ReferenceTemperature",
                                     low_mach_navier_stokes,
                                     (*this),
                                     _T0 );

    MaterialsParsing::read_property( input,
                                     "Physics/"+low_mach_navier_stokes+"/R",
                                     "GasConstant",
                                     low_mach_navier_stokes,
                                     (*this),
                                     _R );

    _p0_over_R = _p0/_R;

    _enable_thermo_press_calc = input("Physics/"+low_mach_navier_stokes+"/enable_thermo_press_calc", false );

    // Read gravity vector
    unsigned int g_dim = input.vector_variable_size("Physics/"+low_mach_navier_stokes+"/g");

    _g(0) = input("Physics/"+low_mach_navier_stokes+"/g", 0.0, 0 );
    _g(1) = input("Physics/"+low_mach_navier_stokes+"/g", 0.0, 1 );
  
    if( g_dim == 3)
      _g(2) = input("Physics/"+low_mach_navier_stokes+"/g", 0.0, 2 );
  
    return;
  }

  template<class Mu, class SH, class TC>
  void LowMachNavierStokesBase<Mu,SH,TC>::init_variables( libMesh::FEMSystem* system )
  {
    // Get libMesh to assign an index for each variable
    this->_dim = system->get_mesh().mesh_dimension();

    this->_flow_vars.init(system);
    this->_temp_vars.init(system);

    /* If we need to compute the thermodynamic pressure, we force this to be a first
       order scalar variable. */
    if( _enable_thermo_press_calc )
      _p0_var.init(system);

    return;
  }

  template<class Mu, class SH, class TC>
  void LowMachNavierStokesBase<Mu,SH,TC>::set_time_evolving_vars( libMesh::FEMSystem* system )
  {
    const unsigned int dim = system->get_mesh().mesh_dimension();

    system->time_evolving(_flow_vars.u_var());
    system->time_evolving(_flow_vars.v_var());

    if (dim == 3)
      system->time_evolving(_flow_vars.w_var());

    system->time_evolving(_temp_vars.T_var());
    system->time_evolving(_flow_vars.p_var());

    if( _enable_thermo_press_calc )
      system->time_evolving(_p0_var.p0_var());

    return;
  }

  template<class Mu, class SH, class TC>
  void LowMachNavierStokesBase<Mu,SH,TC>::init_context( AssemblyContext& context )
  {
    // We should prerequest all the data
    // we will need to build the linear system
    // or evaluate a quantity of interest.
    context.get_element_fe(_flow_vars.u_var())->get_JxW();
    context.get_element_fe(_flow_vars.u_var())->get_phi();
    context.get_element_fe(_flow_vars.u_var())->get_dphi();
    context.get_element_fe(_flow_vars.u_var())->get_xyz();

    context.get_element_fe(_temp_vars.T_var())->get_JxW();
    context.get_element_fe(_temp_vars.T_var())->get_phi();
    context.get_element_fe(_temp_vars.T_var())->get_dphi();
    context.get_element_fe(_temp_vars.T_var())->get_xyz();

    context.get_element_fe(_flow_vars.p_var())->get_phi();
    context.get_element_fe(_flow_vars.p_var())->get_xyz();

    return;
  }

  template<class Mu, class SH, class TC>
  void LowMachNavierStokesBase<Mu,SH,TC>::register_parameter
    ( const std::string & param_name,
      libMesh::ParameterMultiAccessor<libMesh::Number> & param_pointer )
    const
  {
    ParameterUser::register_parameter(param_name, param_pointer);
    _mu.register_parameter(param_name, param_pointer);
    _cp.register_parameter(param_name, param_pointer);
    _k.register_parameter(param_name, param_pointer);
  }

} // namespace GRINS

// Instantiate
template class GRINS::LowMachNavierStokesBase<GRINS::ConstantViscosity,GRINS::ConstantSpecificHeat,GRINS::ConstantConductivity>;
