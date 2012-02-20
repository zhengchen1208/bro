// See the file "COPYING" in the main distribution directory for copyright.

#ifndef LOGGING_WRITERFRONTEND_H
#define LOGGING_WRITERFRONTEND_H

#include "Manager.h"

#include "threading/MsgThread.h"

namespace logging  {

class WriterBackend;

/**
 * Bridge class between the logging::Manager and backend writer threads. The
 * Manager instantiates one \a WriterFrontend for each open logging filter.
 * Each frontend in turns instantiates a WriterBackend-derived class
 * internally that's specific to the particular output format. That backend
 * spawns a new thread, and it receives messages from the frontend that
 * correspond to method called by the manager.
 *
 */
class WriterFrontend {
public:
	/**
	 * Constructor.
	 *
	 * type: The backend writer type, with the value corresponding to the
	 * script-level \c Log::Writer enum (e.g., \a WRITER_ASCII). The
	 * frontend will internally instantiate a WriterBackend of the
	 * corresponding type.
	 *
	 * Frontends must only be instantiated by the main thread.
	 */
	WriterFrontend(bro_int_t type);

	/**
	 * Destructor.
	 *
	 * Frontends must only be destroyed by the main thread.
	 */
	virtual ~WriterFrontend();

	/**
	 * Stops all output to this writer. Calling this methods disables all
	 * message forwarding to the backend and stops the backend thread.
	 *
	 * This method must only be called from the main thread.
	 */
	void Stop();

	/**
	 * Initializes the writer.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. If the backend method fails, it
	 * sends a message back that will asynchronously call Disable().
	 *
	 * See WriterBackend::Init() for arguments. The method takes
	 * ownership of \a fields.
	 *
	 * This method must only be called from the main thread.
	 */
	void Init(string path, int num_fields, const threading::Field* const*  fields);

	/**
	 * Write out a record.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. If the backend method fails, it
	 * sends a message back that will asynchronously call Disable().
	 *
	 * As an optimization, if buffering is enabled (which is the default)
	 * this method may buffer several writes and send them over to the
	 * backend in bulk with a single message. An explicit bulk write of
	 * all currently buffered data can be triggered with
	 * FlushWriteBuffer(). The backend writer triggers this with a
	 * message at every heartbeat.
	 *
	 * See WriterBackend::Writer() for arguments (except that this method
	 * takes only a single record, not an array). The method takes
	 * ownership of \a vals.
	 *
	 * This method must only be called from the main thread.
	 */
	void Write(int num_fields, threading::Value** vals);

	/**
	 * Sets the buffering state.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. If the backend method fails, it
	 * sends a message back that will asynchronously call Disable().
	 *
	 * See WriterBackend::SetBuf() for arguments.
	 *
	 * This method must only be called from the main thread.
	 */
	void SetBuf(bool enabled);

	/**
	 * Flushes the output.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. In addition, it also triggers
	 * FlushWriteBuffer(). If the backend method fails, it sends a
	 * message back that will asynchronously call Disable().
	 *
	 * This method must only be called from the main thread.
	 */
	void Flush();

	/**
	 * Triggers log rotation.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. If the backend method fails, it
	 * sends a message back that will asynchronously call Disable().
	 *
	 * See WriterBackend::Rotate() for arguments.
	 *
	 * This method must only be called from the main thread.
	 */
	void Rotate(string rotated_path, double open, double close, bool terminating);

	/**
	 * Finalizes writing to this tream.
	 *
	 * This method generates a message to the backend writer and triggers
	 * the corresponding message there. If the backend method fails, it
	 * sends a message back that will asynchronously call Disable().
	 *
	 * This method must only be called from the main thread.
	 */
	void Finish();

	/**
	 * Explicitly triggers a transfer of all potentially buffered Write()
	 * operations over to the backend.
	 *
	 * This method must only be called from the main thread.
	 */
	void FlushWriteBuffer();

	/**
	 * Disables the writer frontend. From now on, all method calls that
	 * would normally send message over to the backend, turn into no-ops.
	 * Note though that it does not stop the backend itself, use Stop()
	 * to do thast as well (this method is primarily for use as callback
	 * when the backend wants to disable the frontend).
	 *
	 * Disabled frontend will eventually be discarded by the
	 * logging::Manager.
	 *
	 * This method must only be called from the main thread.
	 */
	void SetDisable()	{ disabled = true; }

	/**
	 * Returns true if the writer frontend has been disabled with SetDisable().
	 */
	bool Disabled()	{ return disabled; }

	/**
	 * Returns the log path as passed into the constructor.
	 */
	const string Path() const	{ return path; }

	/**
	 * Returns the number of log fields as passed into the constructor.
	 */
	int NumFields() const	{ return num_fields; }

	/**
	 * Returns a descriptive name for the writer, including the type of
	 * the backend and the path used.
	 *
	 * This method is safe to call from any thread.
	 */
	string Name() const;

	/**
	 * Returns the log fields as passed into the constructor.
	 */
	const threading::Field* const * Fields() const	{ return fields; }

protected:
	friend class Manager;

	WriterBackend* backend;	// The backend we have instanatiated.
	bool disabled;	// True if disabled.
	bool initialized;	// True if initialized.
	bool buf;	// True if buffering is enabled (default).

	string ty_name;	// Name of the backend type. Set by the manager.
	string path;	// The log path.
	int num_fields;	// The number of log fields.
	const threading::Field* const*  fields;	// The log fields.

	// Buffer for bulk writes.
	static const int WRITER_BUFFER_SIZE = 50;
	int write_buffer_pos;	// Position of next write in buffer.
	threading::Value*** write_buffer;	// Buffer of size WRITER_BUFFER_SIZE.
};

}

#endif