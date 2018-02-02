/*
 * Copyright (C) 2017 Fondazione Istituto Italiano di Tecnologia
 * Authors: Stefano Dafarra
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#ifndef FOOTPRINTSINTERPOLATOR_H
#define FOOTPRINTSINTERPOLATOR_H

#include "FootPrint.h"
#include "iDynTree/Core/VectorFixSize.h"
#include "iDynTree/Core/VectorDynSize.h"
#include "iDynTree/Core/Transform.h"
#include "iDynTree/Core/CubicSpline.h"
#include <memory>
#include <vector>

typedef StepList::const_iterator StepsIndex;

enum class StepPhase{
    Stance,
    SwitchIn,
    SwitchOut,
    Swing
};

typedef struct{
    double initialPosition;
    double initialVelocity;
    double initialAcceleration;
} InitialState;

class FeetInterpolator {

    std::vector<StepsIndex> m_orderedSteps;
    FootPrint m_left, m_right;

    //Step phase related variables
    std::shared_ptr<std::vector<StepPhase> > m_lFootPhases, m_rFootPhases;
    std::vector<size_t> m_phaseShift; //it stores the indeces when a change of phase occurs. The last element is the dimension of m_lFootPhases. It is common to both feet.
    std::vector<size_t> m_mergePoints; //it stores the indeces from which is convenient to merge a new trajectory. The last element is the dimension of m_lFootPhases, i.e. merge after the end
    std::vector<InitialState> m_initStates;
    std::vector<bool> m_lFootContact, m_rFootContact, m_leftFixed;

    //Feet Trajectory related variables
    double m_switchPercentage, m_dT, m_endSwitch, m_initTime, m_stepHeight, m_swingApex;
    std::vector<iDynTree::Transform> m_leftTrajectory, m_rightTrajectory;

    //ZMP related variables
    iDynTree::Vector2 m_leftStanceZMP, m_leftSwitchZMP, m_rightStanceZMP, m_rightSwitchZMP;
    std::vector<double> m_weightInLeft, m_weightInRight;
    std::vector<iDynTree::Vector2> m_leftZMP, m_rightZMP, m_globalZMP;

    //Pause conditions
    double m_maxSwitchTime, m_nominalSwitchTime;
    double m_maxSwingTime, m_nominalSwingTime;
    double m_maxStepTime, m_nominalStepTime;
    bool m_pauseActive;

    //CoM height related variables
    double m_CoMHeight;
    double m_CoMHeightDelta;
    std::vector<double> m_CoMHeightTrajectory, m_CoMHeightAcceleration;

    bool orderSteps();
    bool createPhasesTimings();
    void fillFeetStandingPeriodsVectors();
    void fillLeftFixedVector();
    bool interpolateFoot(const std::vector<StepPhase>& stepPhase, const FootPrint& foot, std::vector<iDynTree::Transform>& output);
    bool computeFootWeightPortion(const std::vector<StepPhase> &stepPhase, const InitialState& alpha0, std::vector<double> &output); //the i-th element in output is in [0,1]
    void mirrorWeightPortion(const std::vector<double>& original, std::vector<double>& mirrored);
    bool computeLocalZMP(const std::vector<StepPhase>& stepPhase,
                         const iDynTree::Vector2 &stanceZmpPosition,
                         const iDynTree::Vector2 &swingZmpInitPosition,
                         std::vector<iDynTree::Vector2> &output);
    void computeGlobalZMP();
    bool computeCoMHeightTrajectory();


public:
    FeetInterpolator();

    bool interpolate(const FootPrint &left, const FootPrint &right, double initTime, double dT,
                     const InitialState &weightInLeftAtMergePoint); //both feet are supposed to start on the ground at zero velocity. The initTime must be greater than the maximum of the first impactTime of the two feet. The first step has half switch time. The FootPrints needs to be ordered!

    bool interpolate(const FootPrint &left, const FootPrint &right, double initTime, double dT);

    //Settings

    bool setSwitchOverSwingRatio(double ratio); //indeed the swing time cannot be null, while the switch time can be very close to zero (but not zero)

    bool setTerminalHalfSwitchTime(double lastHalfSwitchTime); //if not set, it won't bring the ZMP at the center of the feet at the end

    bool setStepHeight(double stepHeight);

    bool setFootApexTime(double swingTimeRatio = 0.5);

    bool setPauseConditions(double maxStepTime, double nominalStepTime);

    bool setStanceZMPDelta(const iDynTree::Vector2& offsetInLeftFootFrame, const iDynTree::Vector2& offsetInRightFootFrame);

    bool setInitialSwitchZMPDelta(const iDynTree::Vector2& offsetInLeftFootFrame, const iDynTree::Vector2& offsetInRightFootFrame); //it is the position the ZMP should have when the switch to the other foot begins.

    bool setCoMHeightSettings(double comHeight, double comHeightStanceDelta); //they are the nominal comHeight and a delta which is summed up during stance phase

    //Getters

    void getFeetTrajectories(std::vector<iDynTree::Transform>& lFootTrajectory, std::vector<iDynTree::Transform>& rFootTrajectory) const;

    void getWeightPercentage(std::vector<double>& weightInLeft, std::vector<double>& weightInRight) const;

    void getZMPTrajectory(std::vector<iDynTree::Vector2>& ZMPTrajectory) const;

    void getLocalZMPTrajectories(std::vector<iDynTree::Vector2>& leftZMPTrajectory, std::vector<iDynTree::Vector2>& rightZMPTrajectory) const;

    void getCoMHeightTrajectory(std::vector<double>& CoMHeightTrajectory) const;

    void getCoMHeightAccelerationProfile(std::vector<double>& CoMHeightAccelerationProfile) const;

    void getFeetStandingPeriods(std::vector<bool>& lFootContacts, std::vector<bool>& rFootContacts) const;

    void getWhenUseLeftAsFixed(std::vector<bool>& leftIsFixed) const;

    void getInitialStatesAtMergePoints(std::vector<InitialState>& initialStates) const;

    void getMergePoints(std::vector<size_t>& mergePoints) const; //indexes in which is suitable to perform a merge of trajectories. The weight percentage is discontinuos in velocity

};

#endif // FOOTPRINTSINTERPOLATOR_H