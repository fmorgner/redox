/**
* Redox test
* ----------
* Increment a key on Redis using an asynchronous command on a timer.
*/

#include <iostream>
#include <vector>
#include "redox.hpp"

using namespace std;
using redox::Redox;
using redox::Command;

double time_s() {
  unsigned long ms = chrono::system_clock::now().time_since_epoch() / chrono::microseconds(1);
  return (double)ms / 1e6;
}

int main(int argc, char* argv[]) {

  Redox rdx = {"localhost", 6379};
  if(!rdx.connect()) return 1;

  if(rdx.set("simple_loop:count", "0")) {
    cout << "Reset the counter to zero." << endl;
  } else {
    cerr << "Failed to reset counter." << endl;
    return 1;
  }

  string cmd_str = "INCR simple_loop:count";
  double freq = 10000; // Hz
  double dt = 1 / freq; // s
  double t = 5; // s
  int parallel = 100;

  cout << "Sending \"" << cmd_str << "\" asynchronously every "
       << dt << "s for " << t << "s..." << endl;

  double t0 = time_s();
  atomic_int count(0);

  vector<Command<int>*> commands;
  for(int i = 0; i < parallel; i++) {
    commands.push_back(&rdx.commandLoop<int>(
        cmd_str,
        [&count, &rdx](Command<int>& c) {
          if (!c.ok()) {
            cerr << "Bad reply: " << c.status() << endl;
          }
          count++;
        },
        dt
    ));
  }

  // Wait for t time, then stop the command.
  this_thread::sleep_for(chrono::microseconds((int)(t*1e6)));
  for(auto& c : commands) c->free();

  double t_elapsed = time_s() - t0;
  double actual_freq = (double)count / t_elapsed;

  // Get the final value of the counter
  long final_count = stol(rdx.get("simple_loop:count"));

  cout << "Sent " << count << " commands in " << t_elapsed << "s, "
       << "that's " << actual_freq << " commands/s." << endl;

  cout << "Final value of counter: " << final_count << endl;

  rdx.disconnect();
  rdx.wait();
  return 0;
}
