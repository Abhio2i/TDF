Component Relationships
=======================

TDF follows a modular architecture where each component has a clear role and communicates over well‑defined interfaces. This design improves maintainability, enables distributed deployment, and allows individual modules to evolve independently.

High‑Level Data Flow
--------------------

1. Users configure entities, scenarios, and runtime parameters in the three editors (Database, Scenario, Runtime).
2. The UI Input Module collects these inputs and forwards them to the Scenario Generation Module and the Simulation (Sensor) Module.
3. The Scenario Generation Module builds or updates scenario definitions and stores them in the Database Module.
4. The Simulation Module reads entity and scenario data from the Database Module, runs the real‑time simulation, and pushes results to the Simulation Output Module.
5. The Simulation Output Module renders visual and analytical outputs, while logs and metrics are stored back into the Database Module for later analysis.

Editor Relationships
--------------------

- The Database Editor manages the master data of entities, platforms, sensors, weapons, formations, radios, and zones used across all scenarios.
- The Scenario Editor consumes definitions from the database, configures missions and environments, and saves the resulting scenarios back into the database.
- The Runtime Editor connects to the running Simulation Module to adjust parameters and entities live; changes are synchronised via UDP and can be persisted in the database.

All three editors run as separate applications that communicate using UDP, keeping entity and scenario data synchronised in real time.

Module‑Level Relationships
--------------------------

- Scenario Generation Module
  - Reads platform and component definitions from the Database Module.
  - Writes fully configured scenarios back to the Database Module.
  - Provides prepared scenario data to the Simulation Module.

- Simulation (Sensor) Module
  - Loads entity, sensor, and weapon definitions plus scenario data from the Database Module.
  - Runs the time‑stepped simulation and updates entity states, sensor outputs, and weapon effects.
  - Sends visual and analytical data to the Simulation Output Module and logs to the Database Module.

- Database Module
  - Acts as the central repository for entities, scenarios, maps, and simulation logs.
  - Serves as the common data source for Database Editor, Scenario Editor, Runtime Editor, Scenario Generation Module, and Simulation Module.

- Simulation Output Module
  - Subscribes to simulation state from the Simulation Module.
  - Pulls supporting data (e.g. maps, entity, shapes) from the Database Module.
  - Produces visual displays and reports for analysis and debriefing.


