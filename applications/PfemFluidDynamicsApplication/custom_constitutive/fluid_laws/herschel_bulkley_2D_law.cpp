//-------------------------------------------------------------
//         ___  __           ___ _      _    _
//  KRATOS| _ \/ _|___ _ __ | __| |_  _(_)__| |
//        |  _/  _/ -_) '  \| _|| | || | / _` |
//        |_| |_| \___|_|_|_|_| |_|\_,_|_\__,_|DYNAMICS
//
//  BSD License:    PfemFluidDynamicsApplication/license.txt
//
//  Main authors:   Alessandro Franci, Ignasi de Pouplana
//  Collaborators:  Timur Tomas
//
//-------------------------------------------------------------
//

// System includes
#include <iostream>

// External includes
#include <cmath>

// Project includes
#include "custom_constitutive/fluid_laws/herschel_bulkley_2D_law.h"
#include "includes/checks.h"
#include "includes/properties.h"
#include "pfem_fluid_dynamics_application_variables.h"

namespace Kratos
{
    //********************************CONSTRUCTOR*********************************
    //****************************************************************************

    HerschelBulkley2DLaw::HerschelBulkley2DLaw() : PfemFluidConstitutiveLaw() {}

    //******************************COPY CONSTRUCTOR******************************
    //****************************************************************************

    HerschelBulkley2DLaw::HerschelBulkley2DLaw(const HerschelBulkley2DLaw &rOther) : PfemFluidConstitutiveLaw(rOther) {}

    //***********************************CLONE************************************
    //****************************************************************************

    ConstitutiveLaw::Pointer HerschelBulkley2DLaw::Clone() const { return Kratos::make_shared<HerschelBulkley2DLaw>(*this); }

    //*********************************DESTRUCTOR*********************************
    //****************************************************************************

    HerschelBulkley2DLaw::~HerschelBulkley2DLaw() {}

    ConstitutiveLaw::SizeType HerschelBulkley2DLaw::WorkingSpaceDimension() { return 2; }

    ConstitutiveLaw::SizeType HerschelBulkley2DLaw::GetStrainSize() { return 3; }

    void HerschelBulkley2DLaw::CalculateMaterialResponseCauchy(Parameters &rValues)
    {
        Flags &r_options = rValues.GetOptions();

        const Properties &r_properties = rValues.GetMaterialProperties();

        Vector &r_strain_vector = rValues.GetStrainVector();
        Vector &r_stress_vector = rValues.GetStressVector();

        const double dynamic_viscosity = this->GetEffectiveMaterialParameter(rValues, DYNAMIC_VISCOSITY);
        const double yield_shear = this->GetEffectiveMaterialParameter(rValues, YIELD_SHEAR);
        const double adaptive_exponent = r_properties[ADAPTIVE_EXPONENT];
        const double flow_index = r_properties[FLOW_INDEX];
        double effective_dynamic_viscosity = 0;

        const double equivalent_strain_rate =
            std::sqrt(2.0 * r_strain_vector[0] * r_strain_vector[0] + 2.0 * r_strain_vector[1] * r_strain_vector[1] +
                      4.0 * r_strain_vector[2] * r_strain_vector[2]);

        // Ensuring that the case of equivalent_strain_rate = 0 is not problematic.
        const double tolerance = 1e-8;
        if (equivalent_strain_rate < tolerance)
        {
            effective_dynamic_viscosity = yield_shear * adaptive_exponent;
        }
        else
        {
            // Se tomará el índice de consistencia como la dynamic_vicosity
            double regularization = 1.0 - std::exp(-adaptive_exponent * equivalent_strain_rate);
            effective_dynamic_viscosity = dynamic_viscosity * std::pow(equivalent_strain_rate, flow_index - 1) + regularization * yield_shear / equivalent_strain_rate;
        }

        const double strain_trace = r_strain_vector[0] + r_strain_vector[1];

        // This stress_vector is only deviatoric
        //  d' = d - I * tr(d)/3
        r_stress_vector[0] = 2.0 * effective_dynamic_viscosity * (r_strain_vector[0] - strain_trace / 3.0);
        r_stress_vector[1] = 2.0 * effective_dynamic_viscosity * (r_strain_vector[1] - strain_trace / 3.0);
        r_stress_vector[2] = 2.0 * effective_dynamic_viscosity * r_strain_vector[2];

        if (r_options.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR))
        {
            this->EffectiveViscousConstitutiveMatrix2D(effective_dynamic_viscosity, rValues.GetConstitutiveMatrix());
        }
    }

    std::string HerschelBulkley2DLaw::Info() const { return "HerschelBulkley2DLaw"; }

    //******************CHECK CONSISTENCY IN THE CONSTITUTIVE LAW******************
    //*****************************************************************************

    int HerschelBulkley2DLaw::Check(const Properties &rMaterialProperties, const GeometryType &rElementGeometry,
                                    const ProcessInfo &rCurrentProcessInfo)
    {

        KRATOS_ERROR_IF(rMaterialProperties[DYNAMIC_VISCOSITY] < 0.0)
            << "Incorrect or missing DYNAMIC_VISCOSITY provided in process info for HerschelBulkley2DLaw: "
            << rMaterialProperties[DYNAMIC_VISCOSITY] << std::endl;

        KRATOS_ERROR_IF(rMaterialProperties[YIELD_SHEAR] < 0.0)
            << "Incorrect or missing YIELD_SHEAR provided in process info for HerschelBulkley2DLaw: "
            << rMaterialProperties[YIELD_SHEAR] << std::endl;

        KRATOS_ERROR_IF(rMaterialProperties[FLOW_INDEX] < 0.0)
            << "Incorrect or missing FLOW_INDEX provided in process info for HerschelBulkley2DLaw: "
            << rMaterialProperties[FLOW_INDEX] << std::endl;

        KRATOS_ERROR_IF(rMaterialProperties[ADAPTIVE_EXPONENT] < 0.0)
            << "Incorrect or missing ADAPTIVE_EXPONENT provided in process info for HerschelBulkley2DLaw: "
            << rMaterialProperties[ADAPTIVE_EXPONENT] << std::endl;

        KRATOS_ERROR_IF(rMaterialProperties[BULK_MODULUS] < 0.0)
            << "Incorrect or missing BULK_MODULUS provided in process info for HerschelBulkley2DLaw: "
            << rMaterialProperties[BULK_MODULUS] << std::endl;

        return 0;
    }

    double HerschelBulkley2DLaw::GetEffectiveMaterialParameter(ConstitutiveLaw::Parameters &rParameters, const Variable<double> &rVariable) const
    {
        return rParameters.GetMaterialProperties()[rVariable];
    }
    
    void HerschelBulkley2DLaw::save(Serializer &rSerializer) const
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS(rSerializer, PfemFluidConstitutiveLaw)
    }

    void HerschelBulkley2DLaw::load(Serializer &rSerializer)
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS(rSerializer, PfemFluidConstitutiveLaw)
    }

} // Namespace Kratos
