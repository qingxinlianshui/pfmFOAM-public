/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016-2017 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "SchneiderbauerEtAlFrictionalStressADM.H"
#include "addToRunTimeSelectionTable.H"
#include "mathematicalConstants.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace ADMdispersedModels
{
namespace frictionalStressModels
{
    defineTypeNameAndDebug(SchneiderbauerEtAl, 0);

    addToRunTimeSelectionTable
    (
        frictionalStressModel,
        SchneiderbauerEtAl,
        dictionary
    );
}
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::SchneiderbauerEtAl
(
    const dictionary& dict
)
:
    frictionalStressModel(dict),
    coeffDict_(dict.optionalSubDict(typeName + "Coeffs")),
    b_("b", dimless, coeffDict_),
    muSt_("muSt", dimless, coeffDict_),
    muC_("muC", dimless, coeffDict_),
    I0_("I0", dimless, coeffDict_),
    aQSk_("aQSk", dimensionSet(1, 0, -2, 0, 0), coeffDict_),
    alphaDeltaMin_("alphaDeltaMin", dimless, coeffDict_)
{

}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::~SchneiderbauerEtAl()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField>
Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::frictionalPressure
(
    const phaseModel& phase,
    const volScalarField& alpha,
    const dimensionedScalar& alphaMinFriction,
    const dimensionedScalar& alphaMax,
    const volScalarField& dp,
    const volScalarField& rho,
    const volSymmTensorField& D
) const
{
//    const volScalarField& alpha1 = phase;
    // Equ. (32)
    return
        2.0*rho*sqr(b_*dp)*min(D&&D,dimensionedScalar("dmax",dimensionSet(0, 0, -2, 0, 0),1000.))
       /sqr(max(alphaMax - alpha, alphaDeltaMin_))
//      +
//       pos(alpha1 - alphaMax)
//      *aQSk_*pow(max(alpha1 - alphaMax, scalar(0)), 0.66)/dp
       ;
}


Foam::tmp<Foam::volScalarField>
Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::frictionalPressurePrime
(
    const phaseModel& phase,
    const volScalarField& alpha,
    const dimensionedScalar& alphaMinFriction,
    const dimensionedScalar& alphaMax,
    const volScalarField& dp,
    const volScalarField& rho,
    const volSymmTensorField& D
) const
{
//    const volScalarField& alpha1 = phase;
    return
         4.0*rho*sqr(b_*dp)*min(D&&D,dimensionedScalar("dmax",dimensionSet(0, 0, -2, 0, 0),1000.))
       / pow3(max(alphaMax - alpha, alphaDeltaMin_))
//      +
//         pos(alpha1 - alphaMax)
//       * 0.66*aQSk_/(pow(max(alpha1 - alphaMax, scalar(1e-8)), 0.33)*dp)
    ;
}


Foam::tmp<Foam::volScalarField>
Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::nu
(
    const phaseModel& phase,
    const volScalarField& alpha,
    const dimensionedScalar& alphaMinFriction,
    const dimensionedScalar& alphaMax,
    const volScalarField& pf,
    const volScalarField& dp,
    const volSymmTensorField& D
) const
{

    tmp<volScalarField> tnu
    (
        new volScalarField
        (
            IOobject
            (
                "SchneiderbauerEtAl:nu",
                phase.mesh().time().timeName(),
                phase.mesh(),
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            phase.mesh(),
            dimensionedScalar("nu", dimensionSet(0, 2, -1, 0, 0), 0.0)
        )
    );

    volScalarField& nuf = tnu.ref();

    forAll(D, celli)
    {
        if (alpha[celli] > alphaMinFriction.value())
        {
            nuf[celli] = (
                             muSt_.value()
                           + (
                                muC_.value() - muSt_.value()
                             )
                            /(
                                 I0_.value()
                               / (
                                   (2.0 * sqrt(0.5*(D[celli]&&D[celli])) * dp[celli])
                                   /(sqrt(pf[celli])+SMALL) + SMALL
                                 )
                               + 1.0
                             )
                          )
                        * pf[celli]
                        / (2.0 * sqrt(0.5*(D[celli]&&D[celli])) + SMALL);
        }
    }

    // Correct coupled BCs
    nuf.correctBoundaryConditions();

    return tnu;
}


bool Foam::ADMdispersedModels::frictionalStressModels::
SchneiderbauerEtAl::read()
{
    coeffDict_ <<= dict_.optionalSubDict(typeName + "Coeffs");

    b_.read(coeffDict_);
    muSt_.read(coeffDict_);
    muC_.read(coeffDict_);

    I0_.read(coeffDict_);
    aQSk_.read(coeffDict_);
    
    alphaDeltaMin_.read(coeffDict_);

    return true;
}


// ************************************************************************* //
