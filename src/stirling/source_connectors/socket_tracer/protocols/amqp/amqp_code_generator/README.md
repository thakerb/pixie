# AMQP Protocol Parsing

Install the requirements via pip install and run
```bash
    python3 amqp_decode_gen.py run
```

The function takes in the `amqp-0-9-1.stripped.xml` from the AMQP specification and generates header and c files for pixie.

The code generation process involves 3 steps:
1. Converting the xml to python objects
2. Generating the strings of all code generation peices
3. Generating the output files(with headers/licenses information)

## Frame Structure
The AMQP structure is split up into 4 types of frames: Method, Header, Body, and Heartbeat.

### Heartbeat Frame
The Heartbeat frame is unique and consists of (frame type, channel, Length)

### Method Frame
The method frame represents an AMQP method such as Publish/Deliver/Ack.
The structure of a method frame consists of (frame type, channel, class id, method id). Depending on the class id and method id, the frame can consist of different arguments

### Content Header Frame
Each AMQP class has a relevant content header frame with different arguments. This usually precedes a Content Body and describes what will be sent.
The structure of a content header frame consists of (frame type, channel, length, class_id, weight, body_size, property_flags, property list)

The content header has special property flags 0xNNNN that dynamically determine if an argument in the property list will show up. For example, if bit 3 is set, the property list will contain the third argument of the class.

### Content Body
These represent the general content body packets that are sent. The structure of a content body frame consists of (frame type, channel, length, body)


## XML structure
The XML holds relevant fields, method ids, and class ids. A sample snippet parsed is below.
```
<amqp major="0" minor="9" revision="1" port="5672">
<class name="connection" handler="connection" index="10">
    <method name="start" synchronous="1" index="10">
      <chassis name="client" implement="MUST"/>
      <response name="start-ok"/>
      <field name="version-major" domain="octet"/>
```
