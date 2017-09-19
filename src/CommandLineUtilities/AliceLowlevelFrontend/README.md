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
* Numbers and addresses are all in base-16. A '0x' prefix for numbers is accepted, but unnecessary for the parameters. 
  The '0x' prefix is omitted for return values.

For example: a register write call could have the argument string "0x504,0x4" meaning write value 0x42 to address 0x4.
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
* Return: SCA command, SCA data

#### SCA write
A basic write to the SCA
* Service type: RPC call
* Service name: SCA_WRITE
* Parameters: SCA command, SCA data
* Return: empty

#### SCA write sequence
Write a sequence of values to the SCA
* Service type: RPC call
* Service name: SCA_SEQUENCE_WRITE
* Parameters: A sequence of pairs of SCA command and data. The pairs are separated by semicolon, the command and data by 
    comma: 
    ~~~
    [command 0],[data 0];[command 1],[data 1]; ... 
    ~~~
    For example:
    ~~~
    10,11;20,21;30,31
    ~~~
* Return: A sequence of SCA read return values corresponding to the commands from the input sequence:
    ~~~
    [value 0];[value 1]; ...
    ~~~
    For example:
    ~~~
    42;123;555
    ~~~
    

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

