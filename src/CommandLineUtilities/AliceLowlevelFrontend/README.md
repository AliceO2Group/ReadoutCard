# ALICE Low-level Front-end (ALF) DIM Server
DIM server for DCS control of the cards


# Usage
`DIM_DNS_NODE=mydimdns roc-alf-server`
ALF will automatically scan the system for cards.


# Services

Service names are organized by card serial number:
`ALF/SERIAL_[serial number]/LINK_[link]/[service name]`

## RPC calls
Some of the services are RPC calls.
* RPC calls take a string containing one or more arguments, and return a string.
* When the RPC has multiple arguments, they must be newline-separated ('\n').
* The return string will contain an 8 character prefix indicating success or failure "success\n" or "failure\n",
  optionally followed by a return value or error message.
* Addresses are all in base-16. A '0x' prefix for numbers is accepted, but unnecessary for the parameters.
  The '0x' prefix is omitted for return values.

For example: a register write call could have the argument string "0x504\n0x4" meaning write value 0x42 to address 0x4.
The return string could be "success\n" or "failure\nAddress out of range".

#### REGISTER_READ
* Service type: RPC call
* Parameters:
  * Register address
* Return: register value

#### REGISTER_WRITE
* Service type: RPC call
* Parameters:
  * Register address
  * Register value
* Return: empty

#### SCA_READ
A basic read from the SCA
* Service type: RPC call
* Parameters: empty
* Return: SCA command and SCA data (comma-separated)

#### SCA_WRITE
A basic write to the SCA
* Service type: RPC call
* Service name: SCA_WRITE
* Parameters:
  * SCA command and SCA data (comma-separated)
* Return: empty

#### SCA_SEQUENCE
Write and read a sequence to the SCA
* Service type: RPC call
* Parameters:
  * A sequence of pairs of SCA command and data. The pairs are separated by newline, the command and data by comma:
    ~~~
    "[command 0],[data 0]\n[command 1],[data 1]\n[etc.]"
    ~~~
    For example:
    ~~~
    "10,a1\n20,b1\n30,c1"
    ~~~
    Comment lines are allowed, they must start with a `#`. For example:
    ~~~
    "# Hello!\n11,22\n33,44\n# Bye!"
    ~~~
* Return: A sequence of SCA read return values corresponding to the commands from the input sequence:
    ~~~
    "[value 0]\n[value 1]\n[etc.]"
    ~~~
    For example:
    ~~~
    "42\n123\n555"
    ~~~
    If an SCA error occurred, the sequence of return values will go up to that point, plus the error message
    If another type of error occurred (such as a formatting error), it will return a failure string.

#### SCA_GPIO_READ
Read the GPIO pins
* Service type: RPC call
* Service name: SCA_GPIO_READ
* Parameters: empty
* Return: SCA data

#### SCA_GPIO_WRITE
Set enabled the selected GPIO pins
* Service type: RPC call
* Service name: SCA_GPIO_WRITE
* Parameters:
  * SCA data
* Return: SCA data

#### PUBLISH_REGISTERS_START
Starts a service that publishes the contents of the given register addresses at the specified interval. 
Values are published as newline separated integers.
The service will have the DNS name: `ALF/SERIAL_[serial number]/LINK_[link]/PUBLISH_REGISTERS/[service name]`.
* Service type: RPC call
* Parameters:
  * Service name
  * Interval in seconds. The server supports intervals with millisecond precision
  * Register addresses. Multiple may be given separated by newlines
* Return: empty

#### PUBLISH_REGISTERS_STOP
Stops a service started with PUBLISH_REGISTERS_START.
* Service type: RPC call
* Parameters:
  * Service name
* Return: empty

#### PUBLISH_SCA_SEQUENCE_START
Starts a service that executes and publishes the results of the given SCA sequence at the specified interval. 
Values are published in the same format as the SCA_SEQUENCE return format.
If an error occurred, the corresponding result will be set to 0xffffffff and the rest of the sequence is aborted.
The service will have the DNS name: `ALF/SERIAL_[serial number]/LINK_[link]/PUBLISH_SCA_SEQUENCE/[service name]`.
* Service type: RPC call
* Parameters:
  * Service name
  * Interval in seconds. The server supports intervals with millisecond precision
  * SCA sequence (for format, see SCA_SEQUENCE parameter format)
* Return: empty

#### PUBLISH_SCA_SEQUENCE_STOP
Stops a service started with PUBLISH_SCA_SEQUENCE_START.
* Service type: RPC call
* Parameters:
  * Service name
* Return: empty

#### CRU_TEMPERATURE
Card's core temperature in degrees Celsius
* Service type: Published
* Value type: double
