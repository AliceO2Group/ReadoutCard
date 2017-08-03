# ALICE Low-level Front-end (ALF) DIM Server
DIM server for DCS control of the cards


# Usage
`DIM_DNS_NODE=mydimdns roc-alf-server --serial=11225`


# Services

Service names are organized by card serial number:
`ALF/SERIAL_[serial number]/[service name]`

## RPC calls
Some of the services are RPC calls.
* RPC calls take a string argument and return a string.
* When the argument must contain multiple values, they must be comma-separated.
* The return string will contain an 8-byte prefix indicating success or failure "success:" or "failure:",
  optionally followed by a return value or error message.
* Numbers and addresses are all in base-10.

For example: a register write call could have the argument string "504,42" meaning write value 42 to address 504.
The return string could be "success:" or "failure:Address out of range".

## Service description

#### Register read
* Service type: RPC call
* Service name: REGISTER_READ
* Parameters: register address
* Return: register value

#### Register write
* Service type: RPC call
* Service name: REGISTER_WRITE
* Parameters: register address, register value
* Return: empty

#### SCA read
A basic read from the SCA
* Service type: RPC call
* Service name: SCA_READ
* Parameters: empty
* Return: SCA data, SCA command

#### SCA write
A basic write to the SCA
* Service type: RPC call
* Service name: SCA_WRITE
* Parameters: SCA data, SCA command
* Return: empty

#### SCA GPIO read
Read the GPIO pins
* Service type: RPC call
* Service name: SCA_GPIO_READ
* Parameters:
* Return: SCA data

#### SCA GPIO write
Set enabled the selected GPIO pins
* Service type: RPC call
* Service name: SCA_GPIO_WRITE
* Parameters: SCA data
* Return: SCA data

#### Temperature
Card's core temperature in degrees Celsius
* Service type: Published
* Service name: TEMPERATURE
* Value type: double

