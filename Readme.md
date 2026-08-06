# Military FPV Drone Simulator

A single-player desktop FPV drone simulator developed as a bachelor's thesis project at Lviv Polytechnic National University.

The project was built in Unreal Engine 4.27 using C++ and Blueprints. It simulates manual FPV drone control, flight physics, telemetry, battery consumption, radio and video signal degradation, AI-controlled targets, vehicle damage and mission-based gameplay.

## Demo and Presentation

- [Watch Gameplay Demo and Presentation](https://drive.google.com/drive/folders/1XuHfmmMX1T_DXhbLsfCRHxJDKrfB3pzo)

## Project Overview

The simulator provides a virtual environment for practicing FPV drone control and completing mission scenarios without using real equipment.

The user controls the drone through a first-person camera and receives flight information through a real-time telemetry HUD. The simulation includes the effects of drone mass, inertia, aerodynamic drag, battery state, radio signal quality, video signal quality and collisions.

The project runs locally and does not connect to real UAVs, real radio systems or external military equipment.

## My Role

**Solo Developer and Tester**

I independently worked on:

- Requirements analysis
- System architecture
- C++ gameplay implementation
- Blueprint configuration and prototyping
- Flight model development and tuning
- User interface implementation
- AI behavior
- Mission design
- Asset integration
- Manual testing
- Defect investigation and fixing
- Regression and performance testing
- Project documentation

## Key Features

### FPV Flight Model

- Custom quadcopter flight model
- Manual throttle, pitch, roll and yaw control
- RC transmitter input through USB
- Drone mass, inertia and center-of-mass configuration
- Aerodynamic drag
- Collision and crash handling
- First-person FPV camera
- Adjustable flight-controller parameters

### Flight Modes

- ACRO
- ANGLE
- HORIZON
- ACRO TRAINER

### Motors and Battery

- Individual motor simulation
- Motor thrust and current consumption
- Battery voltage and remaining capacity
- Voltage drop under load
- Battery influence on available motor power
- Real-time energy consumption data

### Radio and Video Signal

- Control-signal quality based on distance
- Video-signal quality based on distance
- Signal attenuation caused by obstacles
- Radio link quality and failsafe behavior
- Progressive FPV video interference
- Visual noise, scanlines and signal degradation

### Telemetry HUD

The HUD displays real-time information including:

- Flight speed
- Altitude
- Vertical speed
- Heading
- Pitch, roll and yaw
- Flight mode
- Battery voltage
- Current consumption
- Consumed battery capacity
- Control-link quality
- Video-link quality
- Motor activation state
- Payload activation state
- Mission information

### Mission System

The simulator contains three mission scenarios:

1. Destruction of a mortar position
2. Destruction of a self-propelled artillery vehicle
3. Protection of a friendly position from an enemy tank

Each mission includes:

- Mission briefing
- Tactical map
- Primary and optional objectives
- Limited number of drones
- Success and failure conditions
- Score calculation
- Mission timer
- Final result screen

### AI-Controlled Units

The simulator includes AI behavior for:

- Infantry
- Tanks
- Self-propelled artillery
- Armored personnel carriers

AI-controlled units can:

- Follow predefined routes
- Detect and react to the drone
- Change behavior according to mission state
- Attack the drone
- Move toward cover
- Evacuate damaged vehicles
- Retreat after receiving specific damage
- React to nearby impacts and explosions

### Vehicle Damage System

Military vehicles contain functional damage zones, including:

- Engine
- Gun barrel
- Left track
- Right track
- Turret hatch
- Hull hatch

Damage to different zones affects vehicle behavior. Depending on the damaged component, a vehicle can lose mobility, lose the ability to fire, retreat, trigger crew evacuation or be destroyed.

### User Interface

The project includes:

- Main menu
- Mission selection
- Mission briefing screen
- Tactical map
- FPV telemetry HUD
- Pause menu
- Graphics settings
- Flight-controller settings
- Mission result screen

## Technologies

- Unreal Engine 4.27
- C++
- Blueprints
- Unreal Motion Graphics
- Unreal Engine Physics
- RawInput
- Behavior Trees
- Blackboard
- Visual Studio
- Git

## Architecture

The project uses a modular component-based architecture.

The main modules include:

- FPV drone pawn
- Flight controller
- Motor simulation
- Battery simulation
- Radio and video signal simulation
- Telemetry system
- HUD
- Mission controller
- Scoring system
- Vehicle damage system
- Infantry AI
- Vehicle AI
- User interface

C++ is used for the main gameplay logic, flight physics, telemetry, motors, battery, signals, damage and AI systems.

Blueprints are used for mission configuration, level events, routes, UI configuration, visual effects and rapid prototyping.

## Testing

Testing was performed continuously throughout the entire development process.

The project was tested using:

- Functional testing
- Exploratory testing
- Regression testing
- GUI testing
- System-state testing
- Collision testing
- AI behavior testing
- Controller-input testing
- Mission-scenario testing
- Performance testing
- Stability testing

Testing covered:

- Drone controls and flight modes
- Flight-physics behavior
- RC transmitter input
- Motor and battery calculations
- Telemetry updates
- Radio and video signal degradation
- Menu and HUD state transitions
- Mission success and failure conditions
- AI-controlled infantry and vehicles
- Vehicle damage zones
- Object collisions
- NPC navigation
- Visual effects
- Mission scoring and results

Defects were reproduced, investigated, fixed and retested during iterative development.

Examples of identified issues included:

- Incorrect UI state transitions
- HUD remaining visible after leaving a mission
- Pause menu opening outside an active mission
- Settings windows opening in an incorrect layer
- Destroyed vehicles blocking NPC navigation
- Incorrect collision on inactive vehicle components
- Damage zones not following moving vehicle parts
- Visual effects appearing in incorrect positions
- AI state-transition issues
- Incorrect mission-completion conditions
- Flight-model and controller-input inconsistencies

The thesis documentation contains 21 formal test cases:

- 10 functional test cases
- 6 GUI test cases
- 3 system-state and collision test cases
- 2 performance test cases

These test cases represent only the final documented subset of the broader exploratory and iterative testing performed throughout development.

## Requirements Coverage

The documented testing covered the main requirement groups:

- Menus, navigation and tactical map
- Flight controls and flight physics
- Motors, battery and signal simulation
- FPV camera and telemetry HUD
- Mission logic
- AI-controlled targets
- Vehicle damage
- Mission scoring and results

## Running the Project

### Requirements

- Unreal Engine 4.27
- Visual Studio with C++ development tools
- Windows
- Git
- Optional USB RC transmitter

The project was developed using a Jumper T14 transmitter connected through USB in joystick mode.

### Launch

1. Clone the repository:

```bash
git clone https://github.com/ferum55/DroneDiploma.git
```

2. Open the project directory.

3. Open the `.uproject` file using Unreal Engine 4.27.

4. Allow Unreal Engine to rebuild the C++ modules when requested.

5. Open the main level and launch the project through the Unreal Editor.

6. Connect and configure the RC transmitter when physical controller input is required.

Some project assets or local configuration files may require additional setup depending on the development environment.

## Academic Context

The project was created as a bachelor's thesis in Software Engineering at Lviv Polytechnic National University.

The main objective was to design and implement a functional computer simulator that combines:

- Manual FPV piloting
- Real-time flight physics
- RC transmitter input
- Drone-system simulation
- Mission scenarios
- AI-controlled units
- Damage mechanics
- Testing and result evaluation

## Project Status

The simulator is an academic prototype suitable for demonstration and further development.

Possible future improvements include:

- Additional maps and missions
- More drone configurations
- Extended controller configuration
- Improved graphics and effects
- Additional AI behavior
- Automated testing
- VR support
- Multiplayer functionality

## Disclaimer

This project is a virtual academic simulation.

It does not connect to real drones, real weapons, real radio-control systems or external military infrastructure. It was created for educational, research and software-development purposes.
