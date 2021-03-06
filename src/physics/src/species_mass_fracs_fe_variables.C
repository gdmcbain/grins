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
#include "grins/species_mass_fracs_fe_variables.h"

// GRINS
#include "grins/grins_enums.h"
#include "grins/variable_name_defaults.h"
#include "grins/materials_parsing.h"

// libMesh
#include "libmesh/getpot.h"
#include "libmesh/string_to_enum.h"
#include "libmesh/fem_system.h"

namespace GRINS
{
  SpeciesMassFractionsFEVariables::SpeciesMassFractionsFEVariables( const GetPot& input,
                                                                    const std::string& physics_name )
    : SpeciesMassFractionsVariables( input, MaterialsParsing::material_name(input,physics_name) )
  {
    this->_species_FE_family = libMesh::Utility::string_to_enum<GRINSEnums::FEFamily>( input("Physics/"+physics_name+"/species_FE_family", "LAGRANGE") );

    // Read FE family info
    this->_species_order = libMesh::Utility::string_to_enum<GRINSEnums::Order>( input("Physics/"+physics_name+"/species_order", "SECOND") );
  }

  void SpeciesMassFractionsFEVariables::init( libMesh::FEMSystem* system )
  {
    this->_species_vars.resize(this->n_species());
    for( unsigned int s = 0; s < this->n_species(); s++ )
      {
	this->_species_vars[s] = system->add_variable( this->_species_var_names[s],
                                                       this->_species_order,
                                                       this->_species_FE_family);
      }
  }

} // end namespace GRINS
