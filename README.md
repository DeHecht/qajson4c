# Quite-Small JSON for C (qsjson4c)

A simple json library written in C based on qajson4c. Optimized for even lower memory consumption.

[![Build Status](https://travis-ci.org/USESystemEngineeringBV/qajson4c.svg?branch=master)](https://travis-ci.org/USESystemEngineeringBV/qajson4c) [![Build status](https://ci.appveyor.com/api/projects/status/9imof268cwquh463?svg=true)](https://ci.appveyor.com/project/DeHecht/qajson4c) [![codecov](https://codecov.io/gh/USESystemEngineeringBV/qajson4c/branch/master/graph/badge.svg)](https://codecov.io/gh/USESystemEngineeringBV/qajson4c) [![Quality Gate](https://sonarqube.com/api/badges/gate?key=nl.usetechnology.qajson4c-project)](https://sonarqube.com/dashboard/index/nl.usetechnology.qajson4c-project)


## Features

* Extremely low DOM memory consumption.
* Simple API
* Parsing errors (reason and position) can be retrieved from the API.
* Random access on json arrays (they are no linked lists underneath).
* Random access on object members via index.
* API support for parsing DOM directly into a buffer.
	* No need to specify you own malloc/free method.
	* Parser can optimally facilitate buffer memory (no memory is wasted due to overhead)
* Top scores at the [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark) on category ``Parsing Memory``, ``Parsing Memory Peak`` and ``AllocCount`` while scoring quite well at ``Parsing Time`` (Unfortunately the pictures are not yet updated).

## Differences to qajson4c

 * Compression rate about 50% compared to qajson4c.
 * No 64 bit integers supported and only floats are supported instead of doubles.
 * Only parsing json messages is supported.

## Examples

###Example: Parsing without dynamic memory allocation
As shown in the following example. You can directly parse your json into a predefined buffer. Without the need to define a custom malloc, free method.


```

	const size_t BUFF_SIZE = 100;
	char buff[BUFF_SIZE];
	char json[] = "{\"id\":1, \"name\": \"dude\"}";
	const QSJ4C_Value* document;
	size_t buff_len = 0;
	
	buff_len = QSJ4C_parse(json, buff, BUFF_SIZE, &document);
	if (QSJ4C_is_object(document)) {
		QSJ4C_Value* id_node = QSJ4C_object_get(document, "id");
		QSJ4C_Value* name_node = QSJ4C_object_get(document, "name");
		
		if (QSJ4C_is_uint(id_node) && QSJ4C_is_string(name_node)) {
			printf("ID: %u, NAME: %s\n", QSJ4C_get_uint(id_node), QSJ4C_get_string(name_node));
		}
	}
	
```

###Example: Parsing using realloc
As shown in the following example: The library supports parsing with dynamic memory allocation, too. You only need to supply the realloc method or your own custom realloc.


```

	char json[] = "{\"id\":1, \"name\": \"dude\"}";
	const QSJ4C_Value* document = QSJ4C_parse_dynamic(json, realloc);
	if (QSJ4C_is_object(document)) {
		QSJ4C_Value* id_node = QSJ4C_object_get(document, "id");
		QSJ4C_Value* name_node = QSJ4C_object_get(document, "name");
		
		if (QSJ4C_is_uint(id_node) && QSJ4C_is_string(name_node)) {
			printf("ID: %u, NAME: %s\n", QSJ4C_get_uint(id_node), QSJ4C_get_string(name_node));
		}
	}
	
```

