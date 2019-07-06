# thread

1. thread life cycle

- thread is represented by a data structure that can identify a thread and keep track of its usage.

2. how is tread created and managed?

`Fock(proc, args)` : create a thread

- proc is the function to execute
- args is the argument of the function

`thread1 = fock(proc, args)`

> Notice, the fock call here creates a thread
> The unix `Fock` call will create a new process.
> Don't mix them up.

`Join(thread)` : terminate a thread

The call will be terminated until the thread is finished

`children.result = join(thread1)`

> Note, child thread after fock can access the same resources
> as the parent thread within the same process such as virtual
> address mapping and other hardware resources such as memory
> and etc.

Code snippets

```c
Thread thread1;
shared_list list;
thread1 = fock(safe_insert, 4);

safe_insert(6);
join(thread1);
// join is optional as the function(proc) using a shared_list, so we do not need to use
// but join can block here if that's what we want its behavior to be
```
> Order of insert 4 & 6 is undetermined
> Program will block on `join` if insert 6 happens first

3. Mutual Exclusion

`mutex`, `lock(mutex){critical section}`

common thread api;`Lock(mutex); Unlock(mutex)`

```c
// make safe_insert safe
list<int> my_list;
Mutex m;
void safe_insert(int i){
    Lock(m){
        my_list.insert(i);
    }
    // Unlock; Burrell's api for lock
}
```

4. Producer/consumer example

many producer -> one or more consumer

```c
// sudo code
for i=0->10
    producers[i] = fock(safe_insert, NULL)

consumer = fork(print_and_clear, my_list)

// producer : safe insert
Lock(m){
    list->insert(my_thread_id)
}//unlock

// consumer : print_and_clear
Lock(m){
    // this can be wasteful because if consumer locks the mutex and
    // find out list is not full, it is a waste to lock and block
    // other thread execution
    if my_list.full -> print and clear;
    else -> release lock and try again(later)
}
```

5. Conditional variable

```c
// ###############################
// consumer
Lock(m){
    while(my_list.not_full())
Wait(m,list_full); // arg(mutex, conditional_variable)
// wait will automatically release mutex when not satisfy
// conditional variable. And it will automatically acquire
// mutex when conditional_variable satisfies.
    my_list.print_and_remove_all();
}//unlock

// ###############################
// producer
Lock(m){
    my_list.insert(my_thread_id);
    if my_list.full()
        Signal(list_full);
}//unlock
```

**common api**

- `condition type`
- `Wait(mutex, cond)`
  - mutex is automatically released & re-acquired on wait
- `Signal(cond)`
  - notify only one thread waiting on condition
- `Broadcast(cond)`
  - notify all waiting threads

6. Readers/ writer problem

readers : 0 or more can access file

writer : 0 or 1 can access file

if we use the lock operation on the whole file, this will be
too limited as we also locked the file so that only one of the
read can read the file.
`Lock(mutex){file}`
Remember mutex only allow single access. So locking the whole
file seems a bit wasteful.

we can represent our state of reader/writer problem
```c
if((read_counter == 0) && (write_counter ==0))
    ->R ok, W ok
if (read_counter > 0)
    -> R ok
if (writer_counter ==1)
    -> R no, W no
```

```c
resource_counter = 0: free
resource_counter > 0: reading
resource_counter = -1: writing
```

```c
// sample code

// declare
Mutex counter_mutex; // use to protect resource_counter change
Condition read_phase, write_phase;
int resource_counter = 0;

// Reader

// ENTER CRITICAL SECTION
// operation need to do before read operation
Lock(counter_mutex){
    while(resource_counter == -1)
        Wait(counter_mutex, read_phase);
    resource_counter++;
} // unlock

// ... read data ...
// (not in mutex)

// EXIT CRITICAL SECTION
// operation need to do after the read operation
Lock(counter_mutex) {
    resource_counter--;
    if (readers == 0)
        Signal(write_phase);
}// unlock
// End of reader

// WRITER

// ENTER CRITICAL SECTION
// operation need to do before write operation
Lock(counter_mutex){
    while(resource_counter != 0)
        Wait(counter_mutex, write_phase);
    resource_counter = -1;
}// unlock

// ... write data ...
// (not in mutex)

// EXIT CRITICAL SECTION
// operation need to do after write operation
Lock(counter_mutex){
    resource_counter = 0;
    Broadcast(read_phase);
    Signal(write_phase);
}// unlock
```

TYPICAL STRUCTURE OF ENTER CRITICAL SECTION
```c
Lock(counter_mutex){
    while(!predicate_indicating_access_ok)
        wait(mutex, cond_var)
    update_state -> update predicate
}//unlock
```
TYPICAL STRUCTURE OF EXIT CRITICAL SECTION
```c
Lock(mutex){
    update predicate;
    signal/broadcast(cond_var);
}
```

```
ENTER CRITICAL SECTION
per form critical operation(read/write)
EXIT CRITICAL SECTION
```

6. Spurious wake up

```c
// eg OLD WRITER
Lock(counter_mutex){
    resource_counter =0;
    // before we release the mutex
    // we weak up other thread to try to get
    // the mutex
    Broadcast(read_phase);
    Signal(write_phase);
}
// we can change to this

// NEW WRITER
Lock(counter_mutex){
    resource_counter = 0;
}//unlock

Broadcast(read_phase);
Signal(write_phase);
```

But we cannot do that for reader
```c
Lock(counter_mutex){
    resource_counter --;
    //why?
    // because to signal write, we need to access the shared
    // resource counter_resource
    // which will need to be protected by mutex otherwise it
    // will be incorrect.
    if(counter_resource == 0)
        Signal(write_phase);
}//unlock
```