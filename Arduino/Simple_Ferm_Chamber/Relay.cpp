///<summary>This class describes a relay on the system. It can have either
///power going through it or not.</summary>

#include "Relay.h"
#include "Definitions.h"
#include "Utility.h"
#include "Logger.h"

///<summary>Create a relay that is able to activate the given pin output</summary>
///<param name="TriggerPin">The wire on the arduino board to activate</param>
///<param name="DIsplayPin">The wire on the arudiono board to activate for displaying the relay is active</param>
Relay::Relay(int TriggerPin, int DisplayPin, ID* InID) : Triggerable(TriggerPin, DisplayPin)
{
    m_TriggerPin = TriggerPin;
    m_DisplayPin = DisplayPin;
    m_ID = InID;
    m_MinRunTime = INVALID_TIME;
    m_CompressorDelay = INVALID_TIME;
    m_OffTimeStart = INVALID_TIME;
    m_OnTimeStart = INVALID_TIME;
    m_IsOn = false;

    //Make sure the relay's off
    TriggerLow();
    m_IsOn = false;

}

///<summary>Create a relay that is able to activate the given pin output
///This relay will activate for a given minimum amount of time, and will not
///reactivate after being turned off for a given amount of time.  This is used
///to prevent excessive compressor cycling and inhibit damage to the system.</summary>
///<param name="TriggerPin">The wire on the arduino board to activate</param>
///<param name="DIsplayPin">The wire on the arudiono board to activate for displaying the relay is active</param>
///<param name="MinRunTime">The minimum amount of time to run the relay, measured in ms</param>
///<param name="CompressorDelay">How long to wait before activating the relay after turning it off, measured in ms</param>
Relay::Relay(int TriggerPin, 
			 int DisplayPin, ID* InID,  
			 unsigned long MinRunTime, 
			 unsigned long CompressorDelay) : Triggerable(TriggerPin, DisplayPin)
{
    m_TriggerPin = TriggerPin;
    m_DisplayPin = DisplayPin;
    m_ID = InID;
    m_MinRunTime = MinRunTime;
    m_CompressorDelay = CompressorDelay;
    m_OffTimeStart = INVALID_TIME;
    m_OnTimeStart = INVALID_TIME;
    
    //Make sure the relay's off
    TriggerLow();
    m_IsOn = false;
}

///<summary>Turn on the relay if it's possible to.</summary>
///<return>True if successful, false otherwise. </return>
bool Relay::TurnOn()
{
	Logger::Log(F("Trying to turn on the relay"), INF);
    bool retVal = false;
    //We need to see if we can activate the cooling
       if(true == IsOn())
       {
		   Logger::Log(F("Already on"), DEB);
           //Nothing to turn on
           retVal = true;
           goto cleanup;
       }
       if(false == CanTurnOn())
       {
           //Can't turn on yet
		   Logger::PrependLogStatement(WAR);		   
		   Logger::LogStatement(F("Can't turn on "), WAR);
           Logger::LogStatement(m_ID->GetName(), WAR);
		   Logger::EndLogStatement(WAR);
           Utility::Flash(GetDisplayPin(), 5);
           goto cleanup;
       }
         
    TriggerHigh();
    m_IsOn = true;
    retVal = true;

    //Set the time we turned on
    m_OnTimeStart = millis();
cleanup:
    return retVal;
}

///<summary>Turn off the relay if it's possible to.</summary>
///<return>True if successful, false otherwise. .</return>
bool Relay::TurnOff()
{
    bool retVal = false;

	Logger::Log(F("Trying to turn off the relay"), DEB);

    if(false == IsOn())
    {
        //Nothing to turn off.  Success
        Logger::Log(F("Nothing to turn off"), INF);        
        retVal = true;
        goto cleanup;
    }
    if(false == CanTurnOff())
    {
        //Unable to turn the heater off         
        Logger::PrependLogStatement(WAR);		   
		Logger::LogStatement(F("Can't turn off "), WAR);
        Logger::LogStatement(m_ID->GetName(), WAR);
		Logger::EndLogStatement(WAR);        

        Utility::Flash(GetDisplayPin(), 5);
        goto cleanup;
    }
           
    TriggerLow();
    m_IsOn = false;

    //Set the time we turned off
    m_OffTimeStart = millis();
    retVal = true;
cleanup:
    return false;
}

///<summary>Determine if it's possible to turn on the relay.</summary>
///<return>True if the relay can be turned on, false otherwise</return>
bool Relay::CanTurnOn()
{
    bool retVal = false;
    unsigned long difference = 0;    
    //See if we're already on
    if(true == m_IsOn)
    {
        //Already on.  We can't get any more on
        goto cleanup;
    }

    //See if we have a minimum wait time
    if(INVALID_TIME == m_CompressorDelay)
    {
        //We don't need to wait to turn this on
        retVal = true;
        goto cleanup;
    }

    //Get the current time difference
	Logger::PrependLogStatement(DEB);		   
	Logger::LogStatement(F("Off start time is <"), DEB);
    Logger::LogStatement(m_OffTimeStart, DEB);
	Logger::LogStatement(F(">"), DEB);
	Logger::EndLogStatement(DEB);

    difference = Utility::TimeDifference(m_OffTimeStart);

    //See if we've exceeded the necessary wait limit
    if(difference > m_CompressorDelay)
    {
        //We've waited long enough, turn on the relay
        retVal = true;
        goto cleanup;
    }

cleanup:
    return retVal;
}

///<summary>Print out the state of the relay in the following format:
///Name:On|Off</summary>
void Relay::Print()
{	
	Logger::LogCommunicationStatement(m_ID->GetName(), IsOn() ? "On" : "Off");	
}


///<summary>Determine if it's possible to turn off the relay.</summary>
///<return>True if the relay can be turned off, false otherwise</return>
bool Relay::CanTurnOff()
{
    bool retVal = false;
    unsigned long difference = 0;

    if(false == m_IsOn)
    {
        //Already off.  We can't get any more off
        goto cleanup;
    }

    //See if we have a minimum wait time
    if(INVALID_TIME == m_MinRunTime)
    {
        //We don't need to wait to turn this on
        retVal = true;
        goto cleanup;
    }

    //Get the current time difference
    difference = Utility::TimeDifference(m_OnTimeStart);

    //See if we've exceeded the necessary wait limit
    if(difference > m_MinRunTime)
    {
        //We've waited long enough, turn on the relay
        retVal = true;
        goto cleanup;
    }

cleanup:
    return retVal;
}

