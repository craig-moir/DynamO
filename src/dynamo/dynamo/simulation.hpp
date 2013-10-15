/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.dynamomd.org
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <dynamo/particle.hpp>
#include <dynamo/ensemble.hpp>
#include <dynamo/property.hpp>
#include <dynamo/units/units.hpp>
#include <magnet/function/delegate.hpp>
#include <random>
#include <vector>

namespace dynamo
{  
  class Scheduler;
  class OutputPlugin;
  class Species;
  class BoundaryCondition;
  class Topology;
  class Dynamics;
  class Interaction;
  class IntEvent;
  class Local;
  class LocalEvent;
  class Global;
  class GlobalEvent;
  class System;

  class NEventData;
  class PairEventData;
  class ParticleEventData;

  class IDRange;
  class IDPairRange;


  //! \brief Holds the different phases of the simulation initialisation
  typedef enum 
    {
      START         = 0, /*!< The first phase of the simulation. */
      CONFIG_LOADED = 1, /*!< After the configuration has been loaded. */
      INITIALISED   = 2, /*!< Once the classes have been initialised and
			   the simulation is ready to begin. */
      PRODUCTION    = 3, /*!< The simulation has already begun. */
      ERROR         = 4  /*!< The simulation has failed. */
    } ESimulationStatus;
  
  typedef std::mt19937 baseRNG;
  
  /*! \brief Fundamental collection of the Simulation data.
   
    This struct contains all the data belonging to a single
    Simulation.
    
    A pointer to this class has been incorporated in the base classes
    SimBase and SimBase_Const which also provide some general
    std::cout formatting.
   */
  class Simulation : public dynamo::Base
  {
  protected:
    template <class T>
    struct Container: public std::vector<shared_ptr<T> >
    {
      typedef std::vector<shared_ptr<T> > Base;
      using Base::operator[];

      shared_ptr<T>& operator[](const std::string name) {
	for (shared_ptr<T>& ptr : *this)
	  if (ptr->getName() == name) return ptr;
	
	M_throw() << "Could not find the \"" << name << "\" object";
      }

      const shared_ptr<T>& operator[](const std::string name) const {
	for (const shared_ptr<T>& ptr : *this)
	  if (ptr->getName() == name) return ptr;
	
	M_throw() << "Could not find the \"" << name << "\" object";
      }

      typename Base::iterator find(std::string name) {
	for (auto ptr = Base::begin(); ptr != Base::end(); ++ptr)
	  if ((*ptr)->getName() == name) return ptr;
	return Base::end();
      }

      typename Base::const_iterator find(std::string name) const {
	for (auto ptr = Base::begin(); ptr != Base::end(); ++ptr)
	  if ((*ptr)->getName() == name) return ptr;
	return Base::end();
      }
    };

    /*! \brief A class which allows easy selection of Species.
    */
    struct SpeciesContainer: public Container<Species>
    {
      typedef Container<Species> Base;
      using Base::operator[];

      const shared_ptr<Species>& operator[](const Particle& p1) const;
    };

  public:
    /*! \brief Significant default value initialisation.
     */
    Simulation();
    
    /*! \brief Initialise the entire Simulation and the Simulation struct.
     
      Most classes will have an initialisation function and its up to
      this function to call them all and in the right order.
     */
    void initialise();

    /*! \brief Resets the simulation so that it appears to just have
        been loaded from the configuration file.

	This is used when a simulation object is used to run both an
	equilibration run and one or more production runs. Between
	runs you will want to reset the state of the simulation.
    */
    void reset();
    
    SpeciesContainer species;
    void addSpecies(shared_ptr<Species>);

    shared_ptr<BoundaryCondition> BCs;
    
    shared_ptr<Dynamics> dynamics;

    Container<Topology> topology;

    Container<Interaction> interactions;
    
    /*! \brief Determines the Interaction which corresponds to a
        particle pairing.

	\param p1 The first particle in the pair.
	\param p2 The second particle in the pair.
     */
    const shared_ptr<Interaction>& getInteraction(const Particle& p1, const Particle& p2) const;
    
    /*! \brief Determines the next event between a particle pairing.

	\param p1 The first particle in the pair.
	\param p2 The second particle in the pair.
	\return An IntEvent which contains the next Interaction event
	time and type.
     */
    IntEvent getEvent(const Particle& p1, const Particle& p2) const;

    
    /*! \brief Returns the longest-range of the events generated by
        Interactions.
     */
    double getLongestInteraction() const;

    Container<Local> locals;

    Container<Global> globals;
    
    Container<System> systems;

    void stream(const double);

    double calcInternalEnergy() const;

    /*! \brief Sets the Centre of Mass (COM) velocity of the system 
      
       The COM momentum of the system is
      \f[ \bm{P}_{system} = \sum_i m_i \bm{v}_i \f]
      
      We want to first remove any motion of the system, so we subtract
      the COM momentum based on the mass of each particle (E.g. \f$ m_i
      / \sum_j m_j\f$). This has two nice effects, first, particles
      store their velocities, not their momentums so we convert by
      dividing by \f$m_i\f$ which gives

      \f[ \bm{v}_i \to \bm{v}_i -
      (\sum_i m_i \bm{v}_i) / \sum_i m_i \f] 
     
      So relative velocities are preserved as the subtraction is a
      constant for all particles. Also we can now just add the offset to give
     
      \f[ \bm{v}_i \to \bm{v}_i -(\sum_i m_i \bm{v}_i) / \sum_i m_i  + \bm{V}_{COM}\f]
     
      \param COMVelocity The target velocity for the COM of the system.
     */  
    void setCOMVelocity(const Vector COMVelocity = Vector(0,0,0));

    void checkSystem();

    void addSystemTicker();
    
    double getSimVolume() const;
    
    double getNumberDensity() const;
    
    double getPackingFraction() const;

    /*! \brief Finds a plugin of the given type using RTTI.
     */
    template<class T>
    shared_ptr<const T> getOutputPlugin() const
    {
      for (const auto& plugin : outputPlugins)
	if (std::dynamic_pointer_cast<const T>(plugin))
	  return std::static_pointer_cast<const T>(plugin);
      
      return shared_ptr<const T>();
    }

    /*! \brief Finds a plugin of the given type using RTTI.
     */
    template<class T>
    shared_ptr<T> getOutputPlugin()
    {
      for (const shared_ptr<OutputPlugin>& plugin : outputPlugins)
	if (std::dynamic_pointer_cast<T>(plugin))
	  return std::static_pointer_cast<T>(plugin);

      return shared_ptr<T>();
    }    

    /*! Writes the results of the Simulation to a file at the passed
        path.
    
      \param filename The path to the XML file to write (this file
      will either be created or overwritten). The filename must end in
      either ".xml" for uncompressed xml files or ".bz2" for bzip2
      compressed configuration files.
    */
    void outputData(std::string filename = "output.xml.bz2");

    /*! \brief Loads a Simulation from the passed XML file.

      \param filename The path to the XML file to load. The filename
     must end in either ".xml" for uncompressed xml files or ".bz2"
     for bzip2 compressed configuration files.
    */
    void loadXMLfile(std::string filename);
    
    /*! \brief Writes the Simulation configuration to a file at the passed path.

      \param filename The path to the XML file to write (this file
      will either be created or overwritten). The filename must end in
      either ".xml" for uncompressed xml files or ".bz2" for bzip2
      compressed configuration files.

      \param round If true, the data in the XML file will be written
      out at 2 s.f. lower precision to round all the values. This is
      used in the test harness to remove rounding error ready for a
      comparison to a "correct" configuration file.
    */
    void writeXMLfile(std::string filename, bool applyBC = true, bool round = false);

    /*! \brief The Ensemble of the Simulation. */
    shared_ptr<Ensemble> ensemble;

    /*! \brief Main loop for the Simulation.
      
      \param silentMode If true, the periodic output of the simulation
      is supressed.
      
      \return Returns false if the simulation has run out of steps
    */
    bool runSimulationStep(bool silentMode = false);

    /*! \brief Main loop for the Simulation.
      
      \param silentMode If true, the periodic output of the simulation
      is supressed.
    */
    inline void runSimulation(bool silentMode = false)
    {
      while (runSimulationStep(silentMode)) {}
    }
  
    /*! This function makes the Simulation exit the runSimulation loop
      at the next opportunity.
      
      Used when forcing the simulation to stop.
    */
    void simShutdown();
  
    /*! Allows a Engine or the Coordinator class to add an
     OutputPlugin to the Simulation.
     
     \param pluginDescriptor A string identifying a type of
     OutputPlugin to load and the values of any options to be
     parsed. E.g., "Plugin:OptA=1,OptB=2"
    */
    void addOutputPlugin(std::string pluginDescriptor);
  
    //! Sets the frequency of the SysTicker event.
    void setTickerPeriod(double);

    //! Scales the frequency of the SysTicker event by the passed factor.
    void scaleTickerPeriod(double);


    /*! \brief The current system time of the simulation. 
      
      This class is long double to reduce roundoff error as this gets
      very large compared to an events delta t.
     */
    long double systemTime;
    
    /*! \brief Number of events executed.*/
    size_t eventCount;

    /*! \brief Maximum number of events to execute.*/
    size_t endEventCount;

    /*! \brief How many events between periodic output/sampling*/
    size_t eventPrintInterval;
    
    /*! \brief Speeds the Simulation loop by being the next periodic
        output collision number.*/
    size_t nextPrintEvent;

    /*! \brief Number of Particle's in the system. */
    size_t N() const { return particles.size(); }
    
    /*! \brief The Particle's of the system. */
    std::vector<Particle> particles;  
    
    /*! \brief A ptr to the Scheduler of the system. */
    shared_ptr<Scheduler> ptrScheduler;
    
    /*! The property store, a list of properties the particles have. */
    PropertyStore _properties;

    /*! \brief The size of the primary image/cell of the simulation. */
    Vector  primaryCellSize;

    /*! \brief The random number generator of the system. */
    mutable baseRNG ranGenerator;
    
    /*! \brief The collection of OutputPlugin's operating on this system.
     */
    std::vector<shared_ptr<OutputPlugin> > outputPlugins; 

    /*! \brief The mean free time of the previous simulation run
     
      This is zero in the case that there is no previous simulation
      data and is already in the units of the simulation once loaded
     */
    double lastRunMFT;

    /*! \brief This is just the ID number of the Simulation when
       multiple are being run at once.
     
      This is used in the EReplicaExchangeSimulation engine.
     */
    size_t simID;
    
    /*! \brief This is the number of replica exchange attempts
       performed in the current simulation.
     
      This is used in the EReplicaExchangeSimulation engine.
     */
    size_t replexExchangeNumber;

    /*! \brief The current phase of the Simulation.
     */
    ESimulationStatus status;

    Units units;    

    void replexerSwap(Simulation&);
    
    /*! \brief Signal on particle changes.
      
      This is used to allow System events to track when a particle is
      being updated. We cannot allow other classes to use this yet, as
      this is swapped during a replica exchange (along with the System
      classes). This must be cleaned up.
     */
    magnet::Signal<void(const NEventData&)> _sigParticleUpdate;

  private:
    size_t _nextPrint;
  };

}
