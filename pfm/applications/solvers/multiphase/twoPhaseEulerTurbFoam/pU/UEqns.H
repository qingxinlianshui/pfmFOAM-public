Info<< "Constructing momentum equations" << endl;

MRF.correctBoundaryVelocity(U1);
MRF.correctBoundaryVelocity(U2);
MRF.correctBoundaryVelocity(U);

fvVectorMatrix U1Eqn(U1, rho1.dimensions()*U1.dimensions()*dimVol/dimTime);
fvVectorMatrix U2Eqn(U2, rho2.dimensions()*U2.dimensions()*dimVol/dimTime);

// modification for periodic box simulations
volScalarField rhoMix = 0.0*rho1;
volVectorField totalMomentum = 0.0*U1;
dimensionedScalar deltaT = U1.mesh().time().deltaT();
if (periodicBox) {
    rhoMix = fvc::domainIntegrate(alpha1*rho1+alpha2*rho2)
                / fvc::domainIntegrate(unity);
    
    totalMomentum =
                 fvc::domainIntegrate(alpha1*rho1*U1 + alpha2*rho2*U2)
                /fvc::domainIntegrate(alpha1*rho1+alpha2*rho2);
}

volScalarField Kd(fluid.Kd());
volVectorField KdUdrift(fluid.KdUdrift());
{

    surfaceScalarField alphaf1("alphaf1", fvc::interpolate(alpha1));
    surfaceScalarField alphaf2("alphaf2", scalar(1) - alphaf1);

    volScalarField Vm(fluid.Vm());

    {
        U1Eqn =
        (
            fvm::ddt(alpha1, rho1, U1) + fvm::div(alphaRhoPhi1, U1)
          - fvm::Sp(contErr1, U1)
          + MRF.DDt(alpha1*rho1 + Vm, U1)
          + phase1.turbulence().divDevRhoReff(U1)
          // Virtual mass force
          + Vm
           *(
                fvm::ddt(U1)
              + fvm::div(phi1, U1)
              - fvm::Sp(fvc::div(phi1), U1)
              - DDtU2
            )
          // drift velocity correction for filtered drag force
          // (ie. similar to turbulent dispersion)
          - KdUdrift
          // own gravity for periodic box
          // (gN = 0, if pimple switch "periodicBox" is false)
          - alpha1*(rho1-rhoMix)*gN
          // in case of periodic box, adjust CG-velocity
          + alpha1*rho1*totalMomentum/deltaT
         ==
            fvOptions(alpha1, rho1, U1)
        );
        U1Eqn.relax();
        U1Eqn += fvm::Sp(Kd, U1);
        fvOptions.constrain(U1Eqn);
        
        // improved momentum predictor
        volScalarField rAU1
        (
            IOobject::groupName("rAU", phase1.name()),
            1.0
           /(
                U1Eqn.A()
            )
        );
        surfaceScalarField rAU1f("rAU1f", fvc::interpolate(rAU1));
        
        surfaceScalarField pPrimef
        (
            fvc::interpolate
            (
                phase1.turbulence().pPrime()*rAU1
            )
        );
        
        if (pimple.momentumPredictor()) {
            solve
            (
                U1Eqn
             ==
                fvc::reconstruct
                (
                    (
                      - pPrimef*fvc::snGrad(alpha1)/rAU1f
                      - alphaf1*fvc::snGrad(p)
                    ) * mesh.magSf()
                  // drag
                  + (fvc::interpolate(rAU1*Kd)*phi2)/rAU1f
                  + fvc::interpolate(alpha1*rho1*rAU1)*(g&mesh.Sf())/rAU1f
                )
            );

        }
        
        U1.correctBoundaryConditions();
        fvOptions.correct(U1);
    }

    {
        U2Eqn =
        (
            fvm::ddt(alpha2, rho2, U2) + fvm::div(alphaRhoPhi2, U2)
          - fvm::Sp(contErr2, U2)
          + MRF.DDt(alpha2*rho2 + Vm, U2)
          + phase2.turbulence().divDevRhoReff(U2)
          // Virtual mass force
          + Vm
           *(
                fvm::ddt(U2)
              + fvm::div(phi2, U2)
              - fvm::Sp(fvc::div(phi2), U2)
              - DDtU1
            )
          // drift velocity correction for filtered drag force
          // (ie. similar to turbulent dispersion)
          + KdUdrift
          // own gravity for periodic box
          // (gN = 0, if pimple switch "periodicBox" is false)
          - alpha2*(rho2-rhoMix)*gN
          // in case of periodic box, adjust CG-velocity
          + alpha2*rho2*totalMomentum/deltaT
         ==
            fvOptions(alpha2, rho2, U2)
        );
        U2Eqn.relax();
        U2Eqn += fvm::Sp(Kd, U2);
        fvOptions.constrain(U2Eqn);
        
        // improved momentum predictor
        // (is currently experimental, deactivate with momentumPredictor "no" in PIMPLE dict)
        volScalarField rAU2
        (
            IOobject::groupName("rAU", phase2.name()),
            1.0
           /(
                U2Eqn.A()
            )
        );
        surfaceScalarField rAU2f("rAU2f", fvc::interpolate(rAU2));
        if (pimple.momentumPredictor()) {
            solve
            (
                U2Eqn
             ==
                fvc::reconstruct
                (
                    (
                      - alphaf2*fvc::snGrad(p)
                    ) * mesh.magSf()
                  // drag
                 + (fvc::interpolate(rAU2*Kd)*phi1)/rAU2f
                 + fvc::interpolate(alpha2*rho2*rAU2)*(g&mesh.Sf())/rAU2f
                )
            );

        }
        
        U2.correctBoundaryConditions();
        fvOptions.correct(U2);
    }
}
