# Quite-Alright JSON for C (qajson4c)

A simple json library written in C. Optimized for low memory consumption and for good usability without dynamic memory allocation.

[![Build Status](https://travis-ci.org/USESystemEngineeringBV/qajson4c.svg?branch=master)](https://travis-ci.org/USESystemEngineeringBV/qajson4c) [![Build status](https://ci.appveyor.com/api/projects/status/9imof268cwquh463/branch/master?svg=true)](https://ci.appveyor.com/project/DeHecht/qajson4c) [![codecov](https://codecov.io/gh/USESystemEngineeringBV/qajson4c/branch/master/graph/badge.svg)](https://codecov.io/gh/USESystemEngineeringBV/qajson4c) [![Quality Gate](https://sonarqube.com/api/badges/gate?key=nl.usetechnology.qajson4c-project)](https://sonarqube.com/dashboard/index/nl.usetechnology.qajson4c-project)


## Features

* Very low DOM memory consumption.
* Simple API
* Parsing errors (reason and position) can be retrieved from the API.
* Random access on json arrays (they are no linked lists underneath).
* Fast access on large objects via json-key (internal hashing/sorting).
* Random access on object members via index.
* API support for parsing DOM directly into a buffer.
	* No need to specify you own malloc/free method.
	* Parser can optimally facilitate buffer memory (no memory is wasted due to overhead)
* Top scores at the [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark) on category ``Parsing Memory``, ``Parsing Memory Peak`` and ``AllocCount`` while scoring quite well at ``Parsing Time`` (Unfortunately the pictures are not yet updated).


## Examples

###Example: Parsing without dynamic memory allocation
As shown in the following example. You can directly parse your json into a predefined buffer. Without the need to define a custom malloc, free method.


```

	const size_t BUFF_SIZE = 100;
	char buff[BUFF_SIZE];
	char json[] = "{\"id\":1, \"name\": \"dude\"}";
	const QAJ4C_Value* document;
	size_t buff_len = 0;
	
	buff_len = QAJ4C_parse(json, buff, BUFF_SIZE, &document);
	if (QAJ4C_is_object(document)) {
		QAJ4C_Value* id_node = QAJ4C_object_get(document, "id");
		QAJ4C_Value* name_node = QAJ4C_object_get(document, "name");
		
		if (QAJ4C_is_uint(id_node) && QAJ4C_is_string(name_node)) {
			printf("ID: %u, NAME: %s\n", QAJ4C_get_uint(id_node), QAJ4C_get_string(name_node));
		}
	}
	
```

###Example: Parsing using realloc
As shown in the following example: The library supports parsing with dynamic memory allocation, too. You only need to supply the realloc method or your own custom realloc.


```

	char json[] = "{\"id\":1, \"name\": \"dude\"}";
	const QAJ4C_Value* document = QAJ4C_parse_dynamic(json, realloc);
	if (QAJ4C_is_object(document)) {
		QAJ4C_Value* id_node = QAJ4C_object_get(document, "id");
		QAJ4C_Value* name_node = QAJ4C_object_get(document, "name");
		
		if (QAJ4C_is_uint(id_node) && QAJ4C_is_string(name_node)) {
			printf("ID: %u, NAME: %s\n", QAJ4C_get_uint(id_node), QAJ4C_get_string(name_node));
		}
	}
	
```

###Example:
It is also possible to create a DOM yourself and to "print" it to a char buffer.

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

The DOM will be created in a buffer (just like with parsing). Strings can be handed over by ref (so the content will not be copied over to save buffer size) or as a copy.

