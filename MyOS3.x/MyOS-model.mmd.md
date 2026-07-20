# MyOS Model (from MyOS-model.xmi)

Converted from the NetBeans/Poseidon UML XMI export (2008).

## Class diagram — model 1.1

```mermaid
classDiagram
  direction TB
  class I_Thread["Thread"] {
    <<interface>>
    +start() void
    +create() void
    +onTimer() void
    +select() void
  }
  class AddressSpace["AddressSpace"] {
    <<interface>>
  }
  class Process["Process"] {
    <<interface>>
  }
  class CPUContext["CPUContext"] {
    <<interface>>
    +setTimer() void
    +getCPUId() void
    +schedulePair(t: Thread) void
  }
  class TimerHeap["TimerHeap"] {
    <<interface>>
  }
  class TimerSource["TimerSource"] {
    <<interface>>
  }
  class ReadyQueue["ReadyQueue"] {
  }
  class C_Thread["Thread"] {
    -cpu_affinity
    -priority
  }
  class Timer["Timer"] {
  }
  class LocalAPIC["LocalAPIC"] {
    +sendIPI() void
  }
  CPUContext "1" *-- "1" TimerHeap
  Process "1" *-- "1" AddressSpace
  CPUContext "1" --> "1" Process : current
  ReadyQueue "0..1" --> "0..*" C_Thread
  C_Thread "1" --> "0..1" C_Thread : pair
  CPUContext "1" --> "0..1" CPUContext : pair
  Process "1" -- "1..*" C_Thread
  TimerHeap "0..1" --> "0..*" Timer
  CPUContext "1" *-- "1" ReadyQueue
  C_Thread "1" o-- "1" Timer
  CPUContext "1" *-- "1" LocalAPIC
  TimerHeap "1" --> "1" TimerSource
  LocalAPIC ..|> TimerSource : realize
  note for ReadyQueue "Prescheduled on specific CPU;  alternative is a global queue from which  all CPUs take the next t..."
  note for C_Thread "Represent logical pairs  (Hyper-Threading / dual-core)  and schedule pairs of threads  (of the sa..."
```

## State diagram — Thread states

```mermaid
stateDiagram-v2
  [*] --> Ready
  Running --> Sleeping : sleep()
  Running --> Suspended : suspend()
  Sleeping --> Ready : timer || wakeup()
  Suspended --> Ready : wakeup()
  Ready --> Running : select
  Running --> [*] : end()
  Running --> Ready : quantum_timer
```

## Sequence diagram — Thread creation & startup

```mermaid
sequenceDiagram
  autonumber
  participant CurrentThread as Current thread
  participant NewThread as New thread
  actor TimerActor as timer
  actor IInterruptHandling as ih
  actor SchedulingPolicy as policy
  actor Scheduler as s
  CurrentThread->NewThread : start
  NewThread-->>CurrentThread : return
  CurrentThread->NewThread : «create»
  NewThread-->>CurrentThread : return
  NewThread->TimerActor : «create»
  TimerActor-->>NewThread : return
  NewThread->TimerActor : trigger
  TimerActor-->>NewThread : return
  TimerActor->IInterruptHandling : softIRQ
  IInterruptHandling-->>TimerActor : return
  IInterruptHandling->IInterruptHandling : lock_irq
  IInterruptHandling->IInterruptHandling : unlock_irq
  IInterruptHandling->TimerActor : onInterrupt
  TimerActor-->>IInterruptHandling : return
  TimerActor->NewThread : onTimer
  NewThread-->>TimerActor : return
  NewThread-->>SchedulingPolicy : return
  SchedulingPolicy-->>NewThread : return
  SchedulingPolicy->SchedulingPolicy : addToReadyList
  SchedulingPolicy-->>SchedulingPolicy : return
  NewThread->Scheduler : reschedule
  Scheduler-->>NewThread : return
  Scheduler->SchedulingPolicy : selectNext
  SchedulingPolicy-->>Scheduler : return
  Scheduler->NewThread : select
  NewThread-->>Scheduler : return
```
