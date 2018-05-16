/**
 * Copyright (C) 2017 Fondazione Istituto Italiano di Tecnologia
 * Authors: Stefano Dafarra
 *          Giulio Romualdi
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */

#include "UnicycleTrajectoryGenerator.h"

bool UnicycleTrajectoryGenerator::clearAndAddMeasuredStep(std::shared_ptr<FootPrint> foot, Step &previousStep, const iDynTree::Vector2 &measuredPosition, double measuredAngle)
{
    foot->clearSteps();

    Step correctedStep = previousStep;

    correctedStep.position = measuredPosition;
    iDynTree::Rotation initialRotation = iDynTree::Rotation::RotZ(previousStep.angle);
    iDynTree::Rotation measuredRotation = iDynTree::Rotation::RotZ(measuredAngle);
    correctedStep.angle = previousStep.angle + (initialRotation.inverse()*measuredRotation).asRPY()(2);

    return foot->addStep(correctedStep);
}

UnicycleTrajectoryGenerator::UnicycleTrajectoryGenerator()
    :m_left(std::make_shared<FootPrint>())
    ,m_right(std::make_shared<FootPrint>())
{
}

bool UnicycleTrajectoryGenerator::generateAndInterpolate(std::shared_ptr<FootPrint> leftFoot, std::shared_ptr<FootPrint> rightFoot, double initTime, double dT,
                                                         const InitialState& weightInLeftAtMergePoint)
{
    m_left = leftFoot;
    m_right = rightFoot;
    return computeNewSteps(leftFoot, rightFoot, initTime) && interpolate(*leftFoot, *rightFoot, initTime, dT, weightInLeftAtMergePoint);
}

bool UnicycleTrajectoryGenerator::generateAndInterpolate(std::shared_ptr<FootPrint> leftFoot, std::shared_ptr<FootPrint> rightFoot, double initTime, double dT)
{
    m_left = leftFoot;
    m_right = rightFoot;
    return computeNewSteps(leftFoot, rightFoot, initTime) && interpolate(*leftFoot, *rightFoot, initTime, dT);
}

bool UnicycleTrajectoryGenerator::generateAndInterpolate(double initTime, double dT, double endTime)
{
    m_left->clearSteps();
    m_right->clearSteps();
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) && interpolate(*m_left, *m_right, initTime, dT);
}

bool UnicycleTrajectoryGenerator::generateAndInterpolate(std::shared_ptr<FootPrint> leftFoot, std::shared_ptr<FootPrint> rightFoot, double initTime, double dT, double endTime)
{
    m_left = leftFoot;
    m_right = rightFoot;
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) && interpolate(*m_left, *m_right, initTime, dT);
}


bool UnicycleTrajectoryGenerator::reGenerate(double initTime, double dT, double endTime, const InitialState &weightInLeftAtMergePoint)
{
    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolate(*m_left, *m_right, initTime, dT, weightInLeftAtMergePoint);
}

bool UnicycleTrajectoryGenerator::reGenerate(double initTime, double dT, double endTime, const InitialState &weightInLeftAtMergePoint,
                                             const Step &measuredLeft, const Step &measuredRight)
{
    Step previousL, previousR;

    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    m_left->getLastStep(previousL);
    m_right->getLastStep(previousR);

    m_left->clearSteps();
    m_right->clearSteps();

    if (!m_left->addStep(measuredLeft)){
        std::cerr << "The measuredLeft step is invalid." << std::endl;
        return false;
    }

    if (!m_right->addStep(measuredRight)){
        std::cerr << "The measuredRight step is invalid." << std::endl;
        return false;
    }

    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolate(*m_left, *m_right, initTime, dT, weightInLeftAtMergePoint, previousL, previousR);
}

bool UnicycleTrajectoryGenerator::reGenerate(double initTime, double dT, double endTime, const InitialState &weightInLeftAtMergePoint,
                                             bool correctLeft, const iDynTree::Vector2 &measuredPosition, double measuredAngle)
{
    Step previousL, previousR, correctedStep;

    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    m_left->getLastStep(previousL);
    m_right->getLastStep(previousR);

    std::shared_ptr<FootPrint> toBeCorrected = correctLeft ? m_left : m_right;

    correctedStep = correctLeft ? previousL : previousR;

    if (!clearAndAddMeasuredStep(toBeCorrected, correctedStep, measuredPosition, measuredAngle)){
        std::cerr << "Failed to update the steps using the measured value." << std::endl;
        return false;
    }

    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
            interpolate(*m_left, *m_right, initTime, dT, weightInLeftAtMergePoint, previousL, previousR);
}

bool UnicycleTrajectoryGenerator::reGenerate(double initTime, double dT, double endTime, const InitialState &weightInLeftAtMergePoint, const iDynTree::Vector2 &measuredLeftPosition, double measuredLeftAngle, const iDynTree::Vector2 &measuredRightPosition, double measuredRightAngle)
{
    Step previousL, previousR;

    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    m_left->getLastStep(previousL);
    m_right->getLastStep(previousR);

    if (!clearAndAddMeasuredStep(m_left, previousL, measuredLeftPosition, measuredLeftAngle)){
        std::cerr << "Failed to update the left steps using the measured value." << std::endl;
        return false;
    }

    if (!clearAndAddMeasuredStep(m_right, previousR, measuredRightPosition, measuredRightAngle)){
        std::cerr << "Failed to update the right steps using the measured value." << std::endl;
        return false;
    }

    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolate(*m_left, *m_right, initTime, dT, weightInLeftAtMergePoint, previousL, previousR);
}


// DCM functions

bool UnicycleTrajectoryGenerator::generateAndInterpolateDCM(double initTime, double dT, double endTime)
{
    m_left->clearSteps();
    m_right->clearSteps();
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) && interpolateDCM(*m_left, *m_right, initTime, dT);
}

bool UnicycleTrajectoryGenerator::generateAndInterpolateDCM(std::shared_ptr<FootPrint> leftFoot, std::shared_ptr<FootPrint> rightFoot, double initTime, double dT, double endTime)
{
    m_left = leftFoot;
    m_right = rightFoot;
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) && interpolateDCM(*m_left, *m_right, initTime, dT);
}


bool UnicycleTrajectoryGenerator::reGenerateDCM(double initTime, double dT, double endTime, const DCMInitialState &DCMBoundaryConditionAtMergePoint)
{
    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolateDCM(*m_left, *m_right, initTime, dT, DCMBoundaryConditionAtMergePoint);
}

bool UnicycleTrajectoryGenerator::reGenerateDCM(double initTime, double dT, double endTime,
                                                const iDynTree::Vector2 &DCMBoundaryConditionAtMergePointPosition,
                                                const iDynTree::Vector2 &DCMBoundaryConditionAtMergePointVelocity,
                                                bool correctLeft,
                                                const iDynTree::Vector2 &measuredPosition, double measuredAngle)
{
    // set the boundary conditions
    DCMInitialState DCMBoundaryConditionAtMergePoint;
    DCMBoundaryConditionAtMergePoint.initialPosition = DCMBoundaryConditionAtMergePointPosition;
    DCMBoundaryConditionAtMergePoint.initialVelocity = DCMBoundaryConditionAtMergePointVelocity;

    // get the right and the foot last steps before the initTime
    Step previousL, previousR;
    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    // get the last steps
    m_left->getLastStep(previousL);
    m_right->getLastStep(previousR);

    Step measuredFoot;
    measuredFoot.position = measuredPosition;
    measuredFoot.angle = measuredAngle;

    if(correctLeft){
        // clear all the steps of the corrected foot
        m_left->clearSteps();

        // get the impact time
        measuredFoot.impactTime = previousL.impactTime;

        // add the steps
        if (!m_left->addStep(measuredFoot)){
            std::cerr << "The measuredLeft step is invalid." << std::endl;
            return false;
        }
    }
    else{
        // clear all the steps of the corrected foot
        m_right->clearSteps();

        // get the impact time
        measuredFoot.impactTime = previousR.impactTime;

        // add the steps
        if (!m_right->addStep(measuredFoot)){
            std::cerr << "The measuredLeft step is invalid." << std::endl;
            return false;
        }
    }

    // evaluate the trajectory
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolateDCM(*m_left, *m_right, initTime, dT, DCMBoundaryConditionAtMergePoint);
}

bool UnicycleTrajectoryGenerator::reGenerateDCM(double initTime, double dT, double endTime,
                                                const iDynTree::Vector2 &DCMBoundaryConditionAtMergePointPosition,
                                                const iDynTree::Vector2 &DCMBoundaryConditionAtMergePointVelocity,
                                                const iDynTree::Vector2 &measuredLeftPosition, double measuredLeftAngle,
                                                const iDynTree::Vector2 &measuredRightPosition, double measuredRightAngle)
{
    // set the boundary conditions
    DCMInitialState DCMBoundaryConditionAtMergePoint;
    DCMBoundaryConditionAtMergePoint.initialPosition = DCMBoundaryConditionAtMergePointPosition;
    DCMBoundaryConditionAtMergePoint.initialVelocity = DCMBoundaryConditionAtMergePointVelocity;

    // get the right and the foot last steps before the initTime
    Step previousL, previousR;
    if (!m_left->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }

    if (!m_right->keepOnlyPresentStep(initTime)){
        std::cerr << "The initTime is not compatible with previous runs. Call a method generateAndInterpolate instead." << std::endl;
        return false;
    }
    m_left->getLastStep(previousL);
    m_right->getLastStep(previousR);

    // clear all the trajectory
    m_left->clearSteps();
    m_right->clearSteps();

    // the new initial steps are the measured steps
    Step measuredLeft, measuredRight;
    measuredLeft.impactTime = previousL.impactTime;
    measuredLeft.position = measuredLeftPosition;
    measuredLeft.angle = measuredLeftAngle;

    measuredRight.impactTime = previousR.impactTime;
    measuredRight.position = measuredRightPosition;
    measuredRight.angle = measuredRightAngle;

    // add the steps
    if (!m_left->addStep(measuredLeft)){
        std::cerr << "The measuredLeft step is invalid." << std::endl;
        return false;
    }

    if (!m_right->addStep(measuredRight)){
        std::cerr << "The measuredRight step is invalid." << std::endl;
        return false;
    }

    // evaluate the trajectory
    return setEndTime(endTime) && computeNewSteps(m_left, m_right, initTime) &&
        interpolateDCM(*m_left, *m_right, initTime, dT, DCMBoundaryConditionAtMergePoint);
}
