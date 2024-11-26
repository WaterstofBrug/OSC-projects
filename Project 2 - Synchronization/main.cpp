// BY Dieks Scholten - s1098110

/**
  * Assignment: synchronization
  * Operating Systems
  */

/**
  Hint: F2 (or Control-klik) on a functionname to jump to the definition
  Hint: Ctrl-space to auto complete a functionname/variable.
  */

// function/class definitions you are going to use
#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;


class Log {
public:
  vector <string> log_vertex;
  mutex log_mtx;

  void write(string str){
    // write a log to the log

    log_mtx.lock();
    try {
      log_vertex.push_back(str); 
      log_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }
  }

  string read() {
    // read last entry from the log
    string ret = "";

    log_mtx.lock();
    try {
      ret = log_vertex[log_vertex.size()-1];
      log_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }

    return ret;
  }

  string read(int index) {
    // read the log at index index
    string ret = "";

    log_mtx.lock();
    try {
      ret = log_vertex[index];
      log_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }

    return ret;
  }

  string read_all() {
    // read the entire log
    string ret = "";

    log_mtx.lock();
    try {
      for (string log: log_vertex) {
        ret = ret + log + "\n";
      }
      log_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }

    return ret;
  }

  void clear() {
    // clear the logger
    log_mtx.lock();
    try {
      log_vertex.erase(log_vertex.begin(), log_vertex.end());
      log_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }
  }
};

class Buffer {
private:
  Log* log_ptr;
public:
  vector <int> buffer;
  mutex buffer_mtx;
  mutex bound_mtx;
  int bound = -1;

  Buffer(Log* logger) : log_ptr(logger) {}

  void write(int num) {
    // add num to the back of the buffer
    bound_mtx.lock();  // >> start of critical section bound
    buffer_mtx.lock();  // >> start of critical section buffer
    
    try {
      if ((int) buffer.size() < bound || bound == -1) { 
        // there is space left in buffer
        buffer.push_back(num);
        
        bound_mtx.unlock();  // >> end of critical section bound
        buffer_mtx.unlock();  // >> end of critical section buffer

        log_ptr->write("WRITE TO BUFFER: SUCCESFULL");
      } else {
        buffer_mtx.unlock();
        bound_mtx.unlock();
        log_ptr->write("WRITE TO BUFFER: FAILED -- buffer full");
      }
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      buffer_mtx.unlock();
      bound_mtx.unlock();
    }
  }

  int read() {
    // read the first element out of the buffer
    buffer_mtx.lock();  // >> start of critical section
    
    try {
      if (buffer.size() > 0) {
        // there are elements to read
        int r = buffer[0];
        buffer.erase(buffer.begin());

        buffer_mtx.unlock();  // >> end of cricital section

        log_ptr->write("READ FROM BUFFER: SUCCESFULL");

        return r;
      }
      buffer_mtx.unlock();
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      buffer_mtx.unlock();
    }

    log_ptr->write("READ FROM BUFFER: FAILED -- buffer empty");

    return -1;
  }

  void set_bound(int new_bound) {
    // change the bound of the buffer
    
    bound_mtx.lock();  // >> start of critical section bound
    buffer_mtx.lock();  // >> start of critical section buffer
    
    try {
      bound = new_bound;
      buffer.resize(bound);

      buffer_mtx.unlock(); // >> end of critical section buffer
      bound_mtx.unlock();  // >> end of critical section bound
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      buffer_mtx.unlock();
      bound_mtx.unlock();
    }

    log_ptr->write("BUFFER RESIZED -- new size: " + to_string(new_bound));
  }

  void un_bound() {
    // make the buffer unbounded
    bound_mtx.lock();  // >> start of critical section bound
    try {
      bound = -1;
      bound_mtx.unlock();  // >> end of critical section bound
    } catch (const exception& e) {
      // in-case of some exception make sure lock is released
      cerr << "Caught exception: " << e.what() << endl;
      bound_mtx.unlock();
    }

    log_ptr->write("BUFFER RESIZED -- new size: unbounded");
  }
};

// define the logger and buffer globally
Log logger;
Buffer buffer(&logger);

// --- FUNCTIONS FOR TESTING ---

void write_50(int id) {
  // write id 50 times to the buffer
  for (int i = 0; i < 50; i++) {
    buffer.write(id);
  }
}

void write_500(int id) {
  // write id 500 times to the buffer
  for (int i = 0; i < 500; i++) {
    buffer.write(id);
  }
}

void write_50_bound(int id, int bound) {
  // bound the buffer to bound and then write id 50 times to the buffer
  buffer.set_bound(80);
  write_50(id);
}

void read_10() {
  // read 10 items from the buffer
  for (int i = 0; i< 10; i++) {
    buffer.read();
  }
}

string read_buffer() {
  // Get all the contents of the buffer
  string ret = "";

  while (buffer.buffer.size() > 0) {
    ret = ret + to_string(buffer.read());
  }
  return ret;
}

void occurences(string str) {
  // Counts all the occurences of each character in a string
  unordered_map<char, int> charCount;

  // Count each character
  for (char ch : str) {
      charCount[ch]++;
  }

  // Print the character counts
  for (const auto &pair : charCount) {
      cout << "Char: " << pair.first << " : " << pair.second << " occurences" << endl;
  }
}

void test0 () {
  // The main thread write 0-9 to the buffer.

  for (int i = 0; i < 10; i++) {
    buffer.write(i);
  }
  cout << "done with test0" << endl;
}

void test1 () {
  // 10 threads simultaneaously write their ID 50 times to the buffer

  vector<thread> threads;
  const int n_threads = 10;

  // let 10 threads run test()
  for (int i = 0; i < n_threads; ++i) {
    threads.push_back(thread(write_50, i));
  }

  for (int i = 0; i < n_threads; ++i) {
    threads[i].join();
  }
}

void test2 () {
  // 10 threads simultaneaously write their ID 500 times to the buffer

  vector<thread> threads;
  const int n_threads = 10;

  // let 10 threads run test()
  for (int i = 0; i < n_threads; ++i) {
    threads.push_back(thread(write_500, i));
  }

  for (int i = 0; i < n_threads; ++i) {
    threads[i].join();
  }
}

void test3 () {
  // 10 threads simultaneaously write their ID 50 times to the buffer
  // 2 threads read 10 entries out of the buffer
  vector<thread> threads;
  const int n_write_threads = 10;
  const int n_read_threads = 2;
  
  for (int i = 0; i < n_write_threads; ++i) {
    threads.push_back(thread(write_50, i));
  }

  for (int i = 0; i < n_read_threads; ++i) {
    threads.push_back(thread(read_10));
  }

  for (int i = 0; i < n_write_threads + n_read_threads; ++i) {
    threads[i].join();
  }
}

void test4() {
  // 10 threads simultaneaously write their ID 50 times to the buffer
  // 1 thread sets the buffer to 80 instead of infinite

  vector<thread> threads;
  const int n_threads = 10;

  // let 10 threads run test()
  for (int i = 0; i < n_threads; ++i) {
    if (i == 6) {
      threads.push_back(thread(write_50_bound, i, 80));
    } else {
      threads.push_back(thread(write_50, i));
    }
  }

  for (int i = 0; i < n_threads; ++i) {
    threads[i].join();
  }
}

// --- MAIN FUNCTION ---

int main(int argc, char* argv[]) {
  // The main functions runs a number of tests.
  
  test0();
  cout << "Result test0: " << read_buffer() << endl;
  cout << logger.read_all() << endl;
  logger.clear();

  test1();
  string r1 = read_buffer();
  cout << "Result test1: " << endl;
  occurences(r1);
  cout << "Buffer: " << r1 << endl;
  cout << endl;

  test2();
  string r2 = read_buffer();
  cout << "Result test2: " << endl;
  occurences(r2);
  cout << "Buffer: " << r2 << endl;
  cout << endl;

  test3();
  string r3 = read_buffer();
  cout << "Result test3, ";
  cout << "length of buffer is: " << to_string(r3.size()) << endl;
  occurences(r3);
  cout << "Buffer: " << r3 << endl;
  cout << endl;

  test4();
  string r4 = read_buffer();
  cout << "Result test4, ";
  cout << "length of buffer is: " << to_string(r4.size()) << endl;
  occurences(r4);

	return 0;
}
