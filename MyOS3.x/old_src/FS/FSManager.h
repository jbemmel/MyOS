/**
 * TODO: This class should read block 0 of a BlockDevice, and determine
 * (automatically or with a hint) which FS to instantiate for that device
 *
 * FS must register with this class using 'registerFS( IFS& fs )', and FSManager
 * will iterate over all registered FS when it is asked to mount a particular
 * device
 */