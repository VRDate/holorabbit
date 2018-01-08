
#include "EasyRabbitWrap.h"

#include "glew.h"

#include <sstream>

//using namespace MNavStealthLink;

extern "C" {
	struct timeval {
		long    tv_sec;         /* seconds */
		long    tv_usec;        /* and microseconds */
	};

	EASYRABBITWRAP_API amqp_connection_state_t * CreateConnection(const char * host, int port)
	{
		amqp_connection_state_t * conn;
		amqp_socket_t *socket = NULL;
		int status;

		conn = new amqp_connection_state_t();

		*conn = amqp_new_connection();

		socket = amqp_tcp_socket_new(*conn);
		if (!socket) {
			return NULL;
		}

		status = amqp_socket_open(socket, host, port);
		if (status) {
			return NULL;
		}

		return conn;
	}

	EASYRABBITWRAP_API bool Login(amqp_connection_state_t * conn, const char * user, const char * pass)
	{
		// Login to the server
		if (isErrorInReply(amqp_login(*conn, "/", 0, 10000000, 0, AMQP_SASL_METHOD_PLAIN, user, pass)))
		{
			return false;
		}

		// Try to open the channel
		amqp_channel_open(*conn, 1);

		if (isErrorInReply(amqp_get_rpc_reply(*conn))) {
			return false;
		}

		return true;
	}


	EASYRABBITWRAP_API amqp_bytes_t * ConnectToExchange(amqp_connection_state_t * conn, const char * exchange, const char * routingkey)
	{
		amqp_bytes_t * queuename = NULL;

		queuename = new amqp_bytes_t();

		amqp_queue_declare_ok_t *r = amqp_queue_declare(*conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
			amqp_empty_table);
		if (isErrorInReply(amqp_get_rpc_reply(*conn))) return NULL;

		*queuename = amqp_bytes_malloc_dup(r->queue);

		if (queuename->bytes == NULL) {
			fprintf(stderr, "Out of memory while copying queue name");
			return NULL;
		}

		amqp_queue_bind(*conn, 1, *queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey),
			amqp_empty_table);

		if (isErrorInReply(amqp_get_rpc_reply(*conn))) return NULL;

		amqp_basic_consume(*conn, 1, *queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
		if (isErrorInReply(amqp_get_rpc_reply(*conn))) {
			return NULL;
		}

		return queuename;
	}


	EASYRABBITWRAP_API bool ConsumeMessage(amqp_connection_state_t * conn, unsigned int * headersize, unsigned char *  header, unsigned int * datasize, unsigned char *  data)
	{
		amqp_rpc_reply_t res;
		amqp_envelope_t envelope;
		timeval timeout;

		timeout.tv_sec = 0;
		timeout.tv_usec = 50;

		amqp_maybe_release_buffers(*conn);
		
		res = amqp_consume_message(*conn, &envelope, &timeout, 0);			

		if (AMQP_RESPONSE_NORMAL != res.reply_type) {
			return false;
		}		

		memcpy(header, envelope.routing_key.bytes, envelope.routing_key.len);
		*headersize = envelope.routing_key.len;			

		////if (memcmp(header, "headlessrendering.bsonframestream.Trajectory1", sizeof(header)) == 0) {
		////	// We have a buffer of dxt1 data

		//GLuint gltex = (GLuint)(size_t)(textureHandle);
		//// Update texture data, and free the memory buffer
		//glBindTexture(GL_TEXTURE_2D, gltex);
		////glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 624, 437, GL_RGBA, GL_UNSIGNED_BYTE, envelope.message.body.bytes);
		//glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 624, 436, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, envelope.message.body.len, envelope.message.body.bytes);
		////delete[](unsigned char*)dataPtr;
		////}
		////else {
		memcpy(data, envelope.message.body.bytes, envelope.message.body.len);
		*datasize = envelope.message.body.len;
		//}


		

				/*printf("Delivery %u, exchange %.*s routingkey %.*s\n",
			(unsigned)envelope.delivery_tag,
			(int)envelope.exchange.len, (char *)envelope.exchange.bytes,
			(int)envelope.routing_key.len, (char *)envelope.routing_key.bytes);

		if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
			printf("Content-type: %.*s\n",
				(int)envelope.message.properties.content_type.len,
				(char *)envelope.message.properties.content_type.bytes);
		}
		printf("----\n");*/

		//amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

		amqp_destroy_envelope(&envelope);

		return true;
	}

	EASYRABBITWRAP_API bool SendRPC(amqp_connection_state_t * conn, const char * exchange, const char * routingkey, const char * messagebody, unsigned int * datasize, unsigned char *  data)
	{
		amqp_bytes_t reply_to_queue;

		amqp_queue_declare_ok_t *r = amqp_queue_declare(*conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
		if (isErrorInReply(amqp_get_rpc_reply(*conn))) return false;

		reply_to_queue = amqp_bytes_malloc_dup(r->queue);

		if (reply_to_queue.bytes == NULL) {
			fprintf(stderr, "Out of memory while copying queue name");
			return false;
		}

		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
			AMQP_BASIC_DELIVERY_MODE_FLAG |
			AMQP_BASIC_REPLY_TO_FLAG |
			AMQP_BASIC_CORRELATION_ID_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
		props.delivery_mode = 2; /* persistent delivery mode */
		props.reply_to = amqp_bytes_malloc_dup(reply_to_queue);
		if (props.reply_to.bytes == NULL) {
			fprintf(stderr, "Out of memory while copying queue name");
			return 1;
		}
		props.correlation_id = amqp_cstring_bytes("1");

		amqp_basic_publish(*conn,
			1,
			amqp_cstring_bytes(exchange),
			amqp_cstring_bytes(routingkey),
			0,
			0,
			&props,
			amqp_cstring_bytes(messagebody) );

		amqp_bytes_free(props.reply_to);

		/*
		wait an answer
		*/

		
			amqp_basic_consume(*conn, 1, reply_to_queue, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
			amqp_get_rpc_reply(*conn);
			amqp_bytes_free(reply_to_queue);

			amqp_frame_t frame;
			int result;

			amqp_basic_deliver_t *d;
			amqp_basic_properties_t *p;
			size_t body_target;
			size_t body_received;

			for (;;) {
				amqp_maybe_release_buffers(*conn);
				result = amqp_simple_wait_frame(*conn, &frame);
				
				printf("Result: %d\n", result);
				if (result < 0) {
					break;
				}

				printf("Frame type: %u channel: %u\n", frame.frame_type, frame.channel);
				if (frame.frame_type != AMQP_FRAME_METHOD) {
					continue;
				}

				printf("Method: %s\n", amqp_method_name(frame.payload.method.id));
				if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD) {
					continue;
				}

				d = (amqp_basic_deliver_t *)frame.payload.method.decoded;
				printf("Delivery: %u exchange: %.*s routingkey: %.*s\n",
					(unsigned)d->delivery_tag,
					(int)d->exchange.len, (char *)d->exchange.bytes,
					(int)d->routing_key.len, (char *)d->routing_key.bytes);

				result = amqp_simple_wait_frame(*conn, &frame);
				if (result < 0) {
					break;
				}

				if (frame.frame_type != AMQP_FRAME_HEADER) {
					fprintf(stderr, "Expected header!");
					abort();
				}
				p = (amqp_basic_properties_t *)frame.payload.properties.decoded;
				if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
					printf("Content-type: %.*s\n",
						(int)p->content_type.len, (char *)p->content_type.bytes);
				}
				printf("----\n");

				body_target = (size_t)frame.payload.properties.body_size;
				body_received = 0;

				while (body_received < body_target) {
					result = amqp_simple_wait_frame(*conn, &frame);
					if (result < 0) {
						break;
					}

					if (frame.frame_type != AMQP_FRAME_BODY) {
						fprintf(stderr, "Expected body!");
						abort();
					}

					body_received += frame.payload.body_fragment.len;					

					
				}

				memcpy(data, frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
				*datasize = frame.payload.body_fragment.len;

				if (body_received != body_target) {
					/* Can only happen when amqp_simple_wait_frame returns <= 0 */
					/* We break here to close the connection */
					break;
				}

				/* everything was fine, we can quit now because we received the reply */
				break;
			}

			return true;

	}


	EASYRABBITWRAP_API void SendString(amqp_connection_state_t * conn, const char * exchange, const char * routingkey, const char * messagebody)
	{
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
		props.delivery_mode = 2; /* persistent delivery mode */			

		if (isError(amqp_basic_publish(*conn,
			1,
			amqp_cstring_bytes(exchange),
			amqp_cstring_bytes(routingkey),
			0,
			0,
			&props,
			amqp_cstring_bytes(messagebody)))) {

		}
	}

	bool isError(int x)
	{
		if (x < 0) {
			return true;
		}

		return false;
	}


	bool isErrorInReply(amqp_rpc_reply_t x)
	{
		switch (x.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			return false;

		case AMQP_RESPONSE_NONE:
			return true;

		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			return true;

		case AMQP_RESPONSE_SERVER_EXCEPTION:
			return true;
		}

		return true;
	}
	//{
	//	std::string hostname = std::string(host);
	//	std::string portname = std::string(port);
	//	gServer = new MNavStealthLink::StealthServer(hostname, portname);

	//	return gServer;
	//}

	//bool TESTDLLSORT_API ConnectToServer(MNavStealthLink::StealthServer * server)
	//{
	//	Error err;
	//	if (!server->connect(err)) {
	//		return false;
	//	}

	//	return true;
	//}

	//void TESTDLLSORT_API DisconnectFromServer(MNavStealthLink::StealthServer * server)
	//{
	//	server->disconnect();
	//}

	//bool TESTDLLSORT_API GetVersionStealth(char * buffer, int len, MNavStealthLink::StealthServer * server)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (server == nullptr) {
	//		strcpy_s(buffer, len, "Server is not created, consider connecting to a server first");
	//		return false;
	//	}
	//	std::ostringstream os;
	//	MNavStealthLink::Version version;
	//	MNavStealthLink::DateTime time;
	//	try {
	//		server->get(version, time);
	//		os << "StealthLink version received from the server: " << version << " at server time " << time;
	//	}
	//	catch (std::exception& e) {
	//		os << "Error getting StealthLink version: " << e.what() << "\r\n";

	//	}

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());
	//	//strcpy_s( buffer, len, "some test string");
	//	return true;
	//}


	//bool TESTDLLSORT_API GetCurrentTask(char * buffer, int len, MNavStealthLink::StealthServer * server)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (server == nullptr) {
	//		strcpy_s(buffer, len, "Server is not created, consider connecting to a server first");
	//		return false;
	//	}

	//	std::ostringstream os;
	//	try {
	//		os << "Current application task is: " << server->getCurrentTask().name;
	//	}
	//	catch (std::exception& e) {
	//		os << "Error getting StealthLink current application task: " << e.what() << "\r\n";

	//	}

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());
	//	return true;
	//}

	//TESTDLLSORT_API int GetAnnotationCount(MNavStealthLink::StealthServer * server)
	//{
	//	std::ostringstream os;
	//	Error err;

	//	AnnotationNameList annotationList;// = new AnnotationNameList();

	//									  // Query for the Annotation Name List.  This call returns a NameList containing
	//									  // the names of all of the available annotations.
	//	if (!server->get(annotationList, err)) {
	//		os << "Error getting the list of annotations: " << err.reason() << "\r\n\r\n";
	//		return 0;
	//	}

	//	return annotationList.size();
	//}

	//TESTDLLSORT_API MNavStealthLink::Annotation** GetAnnotations(MNavStealthLink::StealthServer * server)
	//{
	//	// Query for Annotations. When the call is made without a name the 
	//	// current annotation is returned. An annotation consists of a location
	//	// in Stealth Image Space, in mm, and a note (string).
	//	MNavStealthLink::Annotation** retAnnotations = new Annotation*;

	//	Error err;

	//	AnnotationNameList annotationList; //= new AnnotationNameList();

	//									   // Query for the Annotation Name List.  This call returns a NameList containing
	//									   // the names of all of the available annotations.
	//	if (!server->get(annotationList, err)) {
	//		return nullptr;
	//	}

	//	//retAnnotations = new MNavStealthLink::Annotation*[annotationList->size()];

	//	//// Iterate through all annotation names and query each by name.
	//	//for( int i = 0; i < annotationList->size(); i++ ) {
	//	//	retAnnotations[i] = new Annotation;
	//	//	retAnnotations[i]->name = std::string(annotationList->at(i).c_str());
	//	//	server->get(retAnnotations[i]->name, *retAnnotations[i], err);	

	//	//	if (err) {				
	//	//		return retAnnotations;
	//	//	}			
	//	//}

	//	return retAnnotations;
	//}

	//TESTDLLSORT_API MNavStealthLink::Annotation* GetAnnotation(MNavStealthLink::Annotation** annotationsArray, int index)
	//{
	//	return annotationsArray[index];
	//}

	//TESTDLLSORT_API MNavStealthLink::Annotation* GetAnnotationFromServer(MNavStealthLink::StealthServer * server, int index)
	//{
	//	Error err;

	//	AnnotationNameList annotationList;

	//	// Query for the Annotation Name List.  This call returns a NameList containing
	//	// the names of all of the available annotations.
	//	if (!server->get(annotationList, err)) {
	//		return nullptr;
	//	}

	//	Annotation * retAnnotation = new Annotation(annotationList.at(index));
	//	Annotation anno;
	//	// Try to get the annotation from the server
	//	server->get(annotationList.at(index), *retAnnotation, err);

	//	return retAnnotation;
	//}

	//TESTDLLSORT_API bool GetAnnotationName(char * buffer, int len, MNavStealthLink::Annotation * annotationPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (annotationPtr == nullptr) {
	//		strcpy_s(buffer, len, "Annotation pointer is not valid");
	//		return false;
	//	}

	//	strcpy_s(buffer, len, annotationPtr->name.c_str());
	//	return true;
	//}

	//TESTDLLSORT_API bool GetAnnotationNote(char * buffer, int len, MNavStealthLink::Annotation * annotationPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (annotationPtr == nullptr) {
	//		strcpy_s(buffer, len, "Annotation pointer is not valid");
	//		return false;
	//	}

	//	strcpy_s(buffer, len, annotationPtr->note.c_str());
	//	return true;
	//}

	//TESTDLLSORT_API bool GetAnnotationPoint(char * buffer, int len, MNavStealthLink::Annotation * annotationPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (annotationPtr == nullptr) {
	//		strcpy_s(buffer, len, "Annotation pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;
	//	os << annotationPtr->location.x << "," << annotationPtr->location.y << "," << annotationPtr->location.z;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API MNavStealthLink::NavData* GetNavDataFromServer(MNavStealthLink::StealthServer * server)
	//{
	//	Error err;

	//	MNavStealthLink::NavData * retNavData = new MNavStealthLink::NavData();
	//	// Try to get the annotation from the server
	//	server->get(*retNavData, err);

	//	return retNavData;
	//}

	//TESTDLLSORT_API bool GetNavTip(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;
	//	os << navDataPtr->refExamPosition.tip.x << "," << navDataPtr->refExamPosition.tip.y << "," << navDataPtr->refExamPosition.tip.z;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavHind(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;
	//	os << navDataPtr->refExamPosition.hind.x << "," << navDataPtr->refExamPosition.hind.y << "," << navDataPtr->refExamPosition.hind.z;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavInstrName(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	strcpy_s(buffer, len, navDataPtr->instrumentName.c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavInstrTransform(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->localizer_T_instrument[0][0] << ","
	//		<< navDataPtr->localizer_T_instrument[0][1] << ","
	//		<< navDataPtr->localizer_T_instrument[0][2] << ","
	//		<< navDataPtr->localizer_T_instrument[0][3] << ","
	//		<< navDataPtr->localizer_T_instrument[1][0] << ","
	//		<< navDataPtr->localizer_T_instrument[1][1] << ","
	//		<< navDataPtr->localizer_T_instrument[1][2] << ","
	//		<< navDataPtr->localizer_T_instrument[1][3] << ","
	//		<< navDataPtr->localizer_T_instrument[2][0] << ","
	//		<< navDataPtr->localizer_T_instrument[2][1] << ","
	//		<< navDataPtr->localizer_T_instrument[2][2] << ","
	//		<< navDataPtr->localizer_T_instrument[2][3] << ","
	//		<< navDataPtr->localizer_T_instrument[3][0] << ","
	//		<< navDataPtr->localizer_T_instrument[3][1] << ","
	//		<< navDataPtr->localizer_T_instrument[3][2] << ","
	//		<< navDataPtr->localizer_T_instrument[3][3];

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavInstrGeometryError(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->instGeometryError;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavInstrVisibility(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->instVisibility;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavFrameName(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	strcpy_s(buffer, len, navDataPtr->frameName.c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavFrameTransform(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->frame_T_localizer[0][0] << ","
	//		<< navDataPtr->frame_T_localizer[0][1] << ","
	//		<< navDataPtr->frame_T_localizer[0][2] << ","
	//		<< navDataPtr->frame_T_localizer[0][3] << ","
	//		<< navDataPtr->frame_T_localizer[1][0] << ","
	//		<< navDataPtr->frame_T_localizer[1][1] << ","
	//		<< navDataPtr->frame_T_localizer[1][2] << ","
	//		<< navDataPtr->frame_T_localizer[1][3] << ","
	//		<< navDataPtr->frame_T_localizer[2][0] << ","
	//		<< navDataPtr->frame_T_localizer[2][1] << ","
	//		<< navDataPtr->frame_T_localizer[2][2] << ","
	//		<< navDataPtr->frame_T_localizer[2][3] << ","
	//		<< navDataPtr->frame_T_localizer[3][0] << ","
	//		<< navDataPtr->frame_T_localizer[3][1] << ","
	//		<< navDataPtr->frame_T_localizer[3][2] << ","
	//		<< navDataPtr->frame_T_localizer[3][3];

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavFrameGeometryError(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->frameGeometryError;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	//TESTDLLSORT_API bool GetNavFrameVisibility(char * buffer, int len, MNavStealthLink::NavData * navDataPtr)
	//{
	//	if (len < 60) {
	//		if (len > 12)
	//			strcpy_s(buffer, len, "Make len 500");
	//		return false;
	//	}

	//	if (navDataPtr == nullptr) {
	//		strcpy_s(buffer, len, "NavData pointer is not valid");
	//		return false;
	//	}

	//	std::ostringstream os;

	//	os << navDataPtr->frameVisibility;

	//	strcpy_s(buffer, len, os.str().substr(0, len).c_str());

	//	return true;
	//}

	////TESTDLLSORT_API MNavStealthLink::SurgicalPlan* GetPlanFromServer(MNavStealthLink::StealthServer * server, int index)
	////{
	////	Error err;

	////	AnnotationNameList annotationList;

	////	// Query for the Annotation Name List.  This call returns a NameList containing
	////	// the names of all of the available annotations.
	////	if (!server->get(annotationList, err)) {
	////		return nullptr;
	////	}

	////	Annotation * retAnnotation = new Annotation(annotationList.at(index));
	////	Annotation anno;
	////	// Try to get the annotation from the server
	////	server->get(annotationList.at(index), *retAnnotation, err);

	////	return retAnnotation;
	////}

	////TESTDLLSORT_API bool GetPlanName( char * buffer, int len, MNavStealthLink::NavData * navDataPtr);
	////TESTDLLSORT_API bool GetPlanEntry( char * buffer, int len, MNavStealthLink::NavData * navDataPtr);
	////TESTDLLSORT_API bool GetPlanTarget( char * buffer, int len, MNavStealthLink::NavData * navDataPtr)


}