# Quite-Alright JSON for C (qajson4c)

[![Build Status](https://travis-ci.org/USESystemEngineeringBV/qajson4c.svg?branch=master)](https://travis-ci.org/USESystemEngineeringBV/qajson4c) [![Build status](https://ci.appveyor.com/api/projects/status/9imof268cwquh463?svg=true)](https://ci.appveyor.com/project/DeHecht/qajson4c) [![codecov](https://codecov.io/gh/USESystemEngineeringBV/qajson4c/branch/master/graph/badge.svg)](https://codecov.io/gh/USESystemEngineeringBV/qajson4c) [![Quality Gate](https://sonarqube.com/api/badges/gate?key=nl.usetechnology.qajson4c-project)](https://sonarqube.com/dashboard/index/nl.usetechnology.qajson4c-project)


## Introduction

The main goal of this parser is to provide a thread-safe JSON DOM parsing without dynamic memory allocation.
Another goal is to keep the required buffer size as low as possible.

This is achieved by passing a buffer with a sufficient size to the parse method.

We know that there are a lot mature json parsers around. But when we decided to pick-up the development, there was no parser that suited our needs well. Still we investigated other parsers like cJSON, Parson, jsmn and rapidjson (c++) to get an impression about how other json libraries work internally and use the inspiration for our own development.


###Example:
```

	char buff[100];
	char json[] = "{\"id\":1, \"name\": \"dude\"}";
	const QAJ4C_Value* document;
	
	document = QAJ4C_parse(json, buff, 100);
	if (QAJ4C_is_object(document)) {
		// code retrieving data from the DOM
	}
	
```

As shown in the example above. You have a buffer and a json message and you can directly parse your json into this
buffer. Without the need to define a custom malloc, free method.

It is also possible to create a DOM yourself and to "print" it to a char buffer.

###Example:
```
	
	char buff[2048];
	char outbuff[2048];
	QAJ4C_Builder builder;
	QAJ4C_builder_init(&builder, buff, 2048);

	QAJ4C_Value* root_value = QAJ4C_builder_get_document(&builder);
	QAJ4C_set_object(root_value, 2, &builder);

	QAJ4C_Value* id_value = QAJ4C_object_create_member_by_ref(root_value, "id");
	QAJ4C_set_uint(id_value, 123);

    QAJ4C_Value* name_value = QAJ4C_object_create_member_by_ref(root_value, "name");
    QAJ4C_set_string_ref(name_value, "dude");

	QAJ4C_sprint(root_value, outbuff, 2048);
	printf("Printed: %s\n", outbuff);
	

```

A difference in DOM creation to most other libraries is, that you always need to specify the amount of members of an array or object. But in case you require to use fixed sized buffers, you probably are used to these kind of obstacles.

## Liberal parsing strategy

We wanted to design the library to be liberal with parsing json messages. So qajson4c can also parses following (non-json compliant) constructs.


### Block and Line Comments within json
```

	"{"type":"object", /*** TEST COMMENT **/ "data": {"id": 1}}"

```

### Trailing commas
```

	"{"type":"object", "data": {"id": 1},}"
	
```

But it is also possible to set the parser to strict mode!

