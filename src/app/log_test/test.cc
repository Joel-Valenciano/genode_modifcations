#include <base/log.h>
#include <log_session/connection.h>
#include <base/component.h>

void Component::construct(Genode::Env& env) {
	Genode::Log_connection log(env);
	log.write("awefiwejoew wefjo\n");
}
