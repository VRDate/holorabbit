
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

}
