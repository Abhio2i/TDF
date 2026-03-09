Inspector Overview
==================

The **Inspector Panel** displays detailed properties and configuration values of the selected entity.  
When a user selects an entity from the **Hierarchy Panel**, all its component data—such as model 
properties, physics parameters, sensors, radios, IFF, trajectory, and more—is shown here.

This panel allows users to **view, edit, and fine-tune every parameter** associated with the entity.

.. image:: ../../images/inspector_overview.png
   :alt: Inspector Panel Overview
   :align: center
   :width: 650


Aircraft0_self
~~~~~~~~~~~~~~

Properties of the main entity instance:

- Active  
- Branch  
- Id  
- Iffs  
- Name  
- Parent id  
- Radios  
- Sensors  
- Type  


Transform
---------

.. image:: ../../images/inspector_transform.png
   :alt: Transform Component
   :align: center
   :width: 500

The **Transform** component controls the spatial and orientation properties of the entity:

- Active
- Geocord  
  - Lat  
  - Lon  
  - Alt  
  - Head
- Id
- Position  
  - x  
  - y  
  - z
- Rotation  
  - x  
  - y  
  - z
- Size  
  - x  
  - y  
  - z
- Type  


Collider
--------

.. image:: ../../images/inspector_collider.png
   :alt: Transform Component
   :align: center
   :width: 500
   
The **Collider** component defines the physical collision boundaries of the entity:

- Active  
- Collider (dropdown)  
- Height  
- Length  
- Radius  
- Type  
- Width  


RigidBody
---------

.. image:: ../../images/inspector_rigidbody.png
   :alt: RigidBody Component
   :align: center
   :width: 500

The **RigidBody** component defines motion physics:

- Active  
- AngularVelocity  
  - x  
  - y  
  - z
- AngularDrag  
- DeltaTime  
- Drag  
- FreezePositionX  
- FreezePositionY  
- FreezePositionZ  
- FreezeRotationX  
- FreezeRotationY  
- FreezeRotationZ  
- Gravity  
- Kinematics  
- Mass  
- Type  
- Velocity  
  - x  
  - y  
  - z


DynamicModel
------------

.. image:: ../../images/inspector_dynamicmodel.png
   :alt: Dynamic Model Component
   :align: center
   :width: 500

The **DynamicModel** component defines aerodynamic and control behavior:

- Lift  
- AerodynamicEffect  
- AirBrakesEffect  
- AutoPitchLevel  
- AutoRollLevel  
- BankedTurnEffect  
- Control  
- DragIncreaseFactor  
- MaxEnginePower  
- MoveSpeed  
- PitchEffect  
- RollEffect  
- RotationSpeed  
- StartTurnRadius  
- TurnRadius  
- Type  
- YawEffect  
- ZeroLiftSpeed  


MeshRenderer2D
--------------

.. image:: ../../images/inspector_meshrenderer2d.png
   :alt: Mesh Renderer Component
   :align: center
   :width: 500

The **MeshRenderer2D** component controls visual appearance:

- Active  
- Color  
- Id  
- Sprite  
- Texture  
- Type  


Sensors
-------

.. image:: ../../images/inspector_sensors.png
   :alt: Sensors Component
   :align: center
   :width: 500

- Active  
- Id  
- Sensors  
- Type  


Radios
------

.. image:: ../../images/inspector_radios.png
   :alt: Radios Component
   :align: center
   :width: 500

- Active  
- Id  
- Radios  
- Type  


IFF
---

.. image:: ../../images/inspector_iff.png
   :alt: IFF Component
   :align: center
   :width: 500

- Active  
- Id  
- Iffs  
- Type  


Trajectory
----------

.. image:: ../../images/inspector_trajectory.png
   :alt: Trajectory Component
   :align: center
   :width: 500

- Active  
- Id  
- Type  

