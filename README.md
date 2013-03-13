The Rockstar Halo Finder
========================

Most code: Copyright (C) 2011,2012 Peter Behroozi  
License: GNU GPLv3  
Documentation Paper: [arXiv:1110.4372](http://arxiv.org/abs/1110.4372)


Contents:
---------
*  Compiling
*  Running
    1.  Quick start (single snapshot)
    2.  More complete setup (multiple snapshots/input files)
    3.  Merger Trees
    4.  Host / Subhalo relationships
    5.  Lightcone setup
    6.  Controlling Output Formats
    7.  Infiniband/Network Connectivity Notes
    8.  Full configuration options
    9.  Full example scripts
    10. Example source code for loading particle outputs


Compiling:
----------

If you use the GNU C compiler version 4.0 or above on a 64-bit machine,
compiling should be as simple as typing "make" at the command prompt.

Rockstar does not support compiling on 32-bit machines and has not been
tested with other compilers.  Additionally, Rockstar does not support
non-Unix environments.  (Mac OS X is fine; Windows is not).


Running
-------

1.  ### Quick start (single snapshot):

    Several example configuration files have been provided.  If you have a
    small simulation file and you'd like to run Rockstar on a single processor
    to test its output, edit the file "quickstart.cfg" and change the file
    format to one of `"ASCII"`, `"GADGET"`, `"ART"`, or `"TIPSY"` to match your
    simulation file.  If you use the ART option, only PMss files are
    currently supported.  (PMcrs files, which do not include particle IDs
    explicitly, are not supported).

    If you use `"ASCII"`, `"ART"`, or `"TIPSY"`, you will additionally have to 
    set the particle mass; if you use `"ASCII"` or `"TIPSY"`, you will also 
    have to set parameters for the cosmology and box size.

    If you use `"GADGET"`, you should check to make sure that the length
    and mass conversion multipliers are correct (`GADGET_LENGTH_CONVERSION` and
    `GADGET_MASS_CONVERSION`) to convert Gadget internal units to comoving Mpc/h
    and Msun/h.

    Similarly, if you use TIPSY, you should set the length and velocity
    conversion multipliers (`TIPSY_LENGTH_CONVERSION` and 
    `TIPSY_MASS_CONVERSION`) to convert Tipsy internal units to comoving 
    Mpc/h and physical km/s.

    **Note that TIPSY format is in an alpha stage of support---please contact
    me if you have issues!**

    Then, you can run Rockstar: `./rockstar -c quickstart.cfg <particle file>`
    
    Note that periodic boundary conditions are **not** assumed for a single-cpu
    run.  (For that, see the section on parallel IO).

2.  ### More complete setup (multiple snapshots/input files):

    Note: Rockstar does *not* use MPI for its parallelization.  As such,
    there should be no headaches with finding a working MPI installation
    to use. :)

    For running on multiple snapshots or simulations with multiple input
    files per snapshot, Rockstar uses a master process to direct the
    order of processing steps and several client processes to do the
    actual work.

    For historical reasons, enabling Rockstar to run on multiple input files
    or multiple snapshots is controlled by setting the following "Parallel
    IO" option in the config file:

        PARALLEL_IO = 1

    Several more options are necessary in the configuration file to specify
    filenames, paths, and information about the number of processes you want
    to use.

    By default, the number of reading tasks is set to the number
    of files being read for each simulation snapshot:

        NUM_BLOCKS = <number of files per snapshot>

    However, if you want to have a different (smaller) number of reading
    tasks, you have to use:

    	NUM_READERS = <number of reading tasks>

    (This is helpful if the number of files per snapshot exceeds the maximum
    number of connections allowed by the system on a single socket.)

    You will also have to provide information on where the data files are
    located.  To do so, you will have to set:

        INBASE = "/directory/where/files/are/located"
        FILENAME = "my_sim.<snap>.<block>"

    In this example, `"my_sim"` should be the base of your simulation filename.
    For multiple snapshots, the text `"<snap>"` will be automatically replaced
    by the snapshot number, and the text `"<block>"` will be automatically
    replaced by the block number (`0` to `NUM_BLOCKS-1`).  To specify the range
    of snapshot numbers, you may set

        NUM_SNAPS = <total number of snapshots>
        TARTING_SNAP = <first snap> #defaults to 0
  
    If you have nonstandard names for your snapshots (e.g., `"001"` instead of
    `"1"`), you may create a text file with one snapshot name per line and set:

        SNAPSHOT_NAMES = </path/to/snapshot names>

    This will automatically override the number of snapshots and the starting
    snapshot if specified.  You may do the same thing for the block names,
    too:

        BLOCK_NAMES = </path/to/block names>

    (Note that NUM_BLOCKS should still be specified in this case, so that the
    server knows how many reader connections to accept).

    The number of analysis tasks can be set to either 1 or a multiple of 8.
    If set to 1, periodic boundary conditions are **not** assumed; otherwise
    periodic boundary conditions **are** assumed (see also the `"PERIODIC"` 
    option in the full list of config options).  To set the number of analysis
    tasks, set:

        NUM_WRITERS = <number of analysis tasks>

    Starting Rockstar (under the default options) is then accomplished by
    first starting the server process: `/path/to/rockstar -c server.cfg`
    The server process will generate a file called `"auto-rockstar.cfg"` in the
    data output directory (`OUTBASE`, see below) with contact information;
    you should then start client processes on all the machine(s) necessary
    (`/path/to/rockstar -c OUTBASE/auto-rockstar.cfg`).

    Specifically, you will have to start NUM_READERS (usually, `NUM_BLOCKS`)
    reader tasks and `NUM_WRITERS` writer tasks.  To simplify this process,
    there are two additional options you can set:

        FORK_READERS_FROM_WRITERS = 1
        FORK_PROCESSORS_PER_MACHINE = <number of processors per machine>

    The first option will automatically split off the reader tasks for you
    (which are idle most of the time) so that you only have to start the
    analysis tasks.  The second option will automatically split each analysis
    task into many separate tasks (according to the number of processors
    you specify to use per machine).  Thus, if set, you would only have to
    start Rockstar running once on each machine (instead of once per
    processor).  The command to start Rockstar for the reading/analysis tasks
    is (as explained above): `/path/to/rockstar -c OUTBASE/auto-rockstar.cfg`

    In terms of memory usage, Rockstar will use about 60 bytes / particle
    **maximum total** for a cosmological simulation.  Thus, if you have a
    1024^3 simulation and 2G of memory available per processor, you should
    plan on using at least 32 processors in parallel.  Note that this
    includes the particle readers as well---readers delete particles from
    memory as soon as they are sent to the analysis tasks so that no extra
    memory is wasted.

    If Rockstar is terminated for any reason, it is easy to restart it where
    it left off.  Simply start the server process by running:
    `/path/to/rockstar -c OUTBASE/restart.cfg`.  Then, start client processes
    as you would normally (either from `auto-rockstar.cfg` or your usual
    config file).  Rockstar will automatically start from the last snapshot
    which was incomplete.

3.  ### Merger trees

    By default, the output files for Rockstar include some basic descendant
    information for each halo.  However, it is strongly recommended that
    you use a more sophisticated package, such as 
    [Consistent Trees](http://code.google.com/p/consistent-trees) 
    to improve consistency between timesteps before using the descendant 
    information for science purposes (e.g., calculating merger rates).  
    Rockstar includes a script to autogenerate configuration files for 
    Consistent Trees (see the Consistent Trees `README` file for use).

4.  ### Host / Subhalo relationships

    By default, the output files for Rockstar do not include information
    about which halos are hosts (as opposed to subhalos).  This is because
    the consistent tree code (see Merger trees, above) automatically
    calculates such relationships.  However, if you do not intend to use the
    merger tree code, then there is a utility included with Rockstar to
    postprocess the output halo catalogs (i.e., the `out_*.list` files).  To
    compile, run `make parents` from the Rockstar source directory.  Then,
    run the following command

        /path/to/rockstar/util/find_parents <box_size> out_XYZ.list

    on each halo catalog for which you want to find host halos.  Information
    about which halos are hosts and which are subs will be output as an
    extra column, the parent ID.  Host halos will have a parent ID of -1;
    subhaloes will have a parent ID which corresponds to their host halo ID.

5.  ### Lightcone setup

    Lightcones are currently only supported with parallel IO (see options
    above).  To turn on lightcone support, set

        LIGHTCONE = 1

    The scale factor will be automatically computed (flat cosmologies only)
    from the distance to the origin, which is set by

        LIGHTCONE_ORIGIN = (x,y,z) # default: (0,0,0)

    If you want a lightcone created by joining two different fields of view
    from the same box, you will have to set the following options:

        LIGHTCONE_ALT_SNAPS = /path/to/second_lightcone_snapnames
        LIGHTCONE_ALT_ORIGIN = (x,y,z) #default: (0,0,0)

    (The `LIGHTCONE_ALT_ORIGIN` option translates all particles in the second
    field of view from (x,y,z) to the first `LIGHTCONE_ORIGIN`.)  If using two
    fields of view, it is assumed that they have equal numbers of input files;
    `NUM_BLOCKS` should be set to the combined total number of input files.

    If you have more than two fields of view from the same box, you will
    have to make sure that their positions are already translated
    appropriately so that they can be joined/unioned together without any
    overlap issues.  In this case, you should not set any of the ALT lightcone
    options.  Additionally, you should set `IGNORE_PARTICLE_IDS` = 1 so as
    to prevent confusion with the same particle ID appearing multiple times.

6.  ### Controlling Output Formats:

    Rockstar can currently output in either ascii or binary formats.
    You can control the output directory for halo catalogs by setting:

        OUTBASE = "/desired/output/path" # default is current directory

    By default, both binary and ascii catalogs are printed.  You can change
    this by setting:

        OUTPUT_FORMAT = "BOTH" # or "ASCII" or "BINARY"

    However, as the binary outputs are required for generating merger trees,
    merger trees will *not* be generated if you select the `"ASCII"` option.
    If you want merger trees but not the binary outputs (which take up
    lots of space), you can set the following option:

        DELETE_BINARY_OUTPUT_AFTER_FINISHED = 1

    For the binary outputs, there is a 256-byte header (detailed in
    `io/meta_io.h`) followed by a binary dump of the halo structures
    (see `halo.h`), followed by a binary dump of the particle ids in
    each halo (type `int64_t`).  See the `load_binary_halos()` routine in
    `io/meta_io.c` for an example of how to read in the binary files.

    To change the minimum particle size of output halos, set

        MIN_HALO_OUTPUT_SIZE = <minimum number of particles> #default: 20

    Note that this option does **not** specify the minimum number of particles
    bound within the virial or other halo radius.  Instead, it specifies the
    minimum number of particles which Rockstar identifies as uniquely assigned
    to that halo.  The virial mass can (and usually will) be much less.

    To avoid printing spurious halos, Rockstar excludes significantly 
    unbound objects by default.  To change the threshold of what is considered
    "significantly unbound"---e.g., for studying tidal remnants, you may
    consider lowering the value of the following parameter:      

        UNBOUND_THRESHOLD = <minimum fraction of bound mass> #default: 0.5

    Generally, this affects the halo mass function at the few percent level
    for halos of 100 bound particles or less.

    Rockstar does not by default output full particle information to
    save space.  In order to turn on this capability, set

        FULL_PARTICLE_CHUNKS = <number of processes to output particles>

    to the desired number of analysis processes which you want to output
    particles (i.e., something between `1` and `NUM_WRITERS`) at every timestep.

    Alternately, you can choose to use the BGC2 output format, which
    records full particle information as well as some halo information for
    an entire snapshot.  In order to turn on this capability, set

        BGC2_SNAPNAMES = </path/to/snapshot_names>

    Where the file contains a list of snapshots (formatted the same way as
    in `SNAPSHOT_NAMES`, if you used that option) for which you want BGC2
    output.  (Note that BGC2 output is the only way to obtain a fully SO
    mass calculation, as all other mass calculations are performed on
    FOF groups, which may not include all the particles out to the desired
    mass threshold.)  Note that the BGC2 format is currently incompatible
    with processing lightcones.

    Once the BGC2 files have been generated, a short postprocessing step
    is necessary to bring them into full compliance with the spec (calculating
    subs/centrals, etc.).  You will need to compile the postprocessing
    code with:

        make bgc2

    Then, for every BGC2 snapshot you have generated, you will need to run

        /path/to/rockstar/util/finish_bgc2 -c rockstar.cfg -s <snap>

    Note that particles in BGC2 files are automatically wrapped around
    box edges if periodic boundary conditions are enabled; this way, they
    always form contiguous regions.

7.  ### Infiniband / Network Connectivity Notes:

    As particle transfer is usually a small fraction of the total analysis
    time, Rockstar performs well on pretty much any gigabit or faster network.

    However, if you have IP over Infiniband, Rockstar can use this natively;
    for the server address, you simply have to specify the IP address
    assigned to the Infiniband port.  Alternately, if you have "auto"
    specified for the server name, you may force Rockstar to use the
    Infiniband interface (e.g., `"ib0"`) with the following config option:

        PARALLEL_IO_SERVER_INTERFACE = "ib0"

    Rockstar supports IPv4 and IPv6 natively; addresses may be specified
    in either format.

8.  ### Full Configuration Options:

    For those options not mentioned directly above:

        MASS_DEFINITION = <"vir" or "XXXc" or "XXXb" etc.> #default: vir

    This lets you specify how you want masses calculated.  `"vir"` uses the
    formula from Bryan & Norman 1998; a number plus `"c"` or `"b"` calculates
    masses relative to the critical or background density, respectively.
    (E.g. `"200b"` or `"200c"`, or even `"1600b"`).

        SCALE_NOW = <current cosmological scale factor>
        h0 = <hubble constant today> # in units of 100 km/s/Mpc
        Ol = <Omega_Lambda> # in units of the critical density
        Om = <Omega_Matter> # in units of the critical density

    These cosmology options are only relevant if one is reading from an `ASCII`
    or `TIPSY` particle file.  (For `TIPSY`, the `SCALE_NOW` parameter may be
    omitted).  For other data formats, these values are automatically
    overwritten with values in the particle data files.

        PARTICLE_MASS = <mass of each particle> #in Msun/h

    It is not necessary to specify the particle mass for `GADGET2` files.
    However, for `GADGET2` files, you will need to specify the conversion to
    Rockstar's internal length (comoving Mpc/h) and mass (Msun/h) units:

        GADGET_MASS_CONVERSION = <conversion from GADGET units to Msun/h>
        GADGET_LENGTH_CONVERSION = <conversion from GADGET units to Mpc/h>

    In general, these will be ~1e10 (mass conversion) and either 1 or
    1e-3 (length conversion for Mpc/h and kpc/h, respectively).

    Similarly, for `TIPSY` files, you will need to convert lengths and
    velocities to comoving MPC/h and physical km/s:

        TIPSY_LENGTH_CONVERSION = <conversion from TIPSY units to Mpc/h>
        TIPSY_VELOCITY_CONVERSION = <conversion from TIPSY units to km/s>

    **Note that `TIPSY` format is in an alpha stage of support---please contact
    me if you have issues!**

    To disable periodic boundary conditions (only applicable for `PARALLEL_IO`),
    you can set:
    
        PERIODIC = 0

    **Note that setting this to 1 does not enable periodic boundary conditions
    for single-processor halo finding.**

    	BOX_SIZE = <side length of cosmological box in comoving Mpc/h>

    The box size is only necessary to set for `ASCII` particle data.

    	GADGET_SKIP_NON_HALO_PARTICLES = <0 or 1> #default = 1

    By default, Rockstar only considers dark matter particles; the preceeding
    option can be set to 0 to force consideration of other particles as well
    in `GADGET2` files.  The default halo particle type in `GADGET` is 1; 
    however, if you need to change this, you can use the following option:

        GADGET_HALO_PARTICLE_TYPE = <0 to 5> #default = 1

    Note that Rockstar has no current support for multiple particle masses.

    	RESCALE_PARTICLE_MASSES = <0 or 1> #default 1

    If only dark matter particles are used from `GADGET2` files in a simulation
    which also includes gas particles, it is necessary to rescale the particle
    masses so as to preserve the correct matter density; setting this option
    tells Rockstar to do so.

    If for some reason your simulation data has inconsistent or duplicate
    particle IDs, you can set the following option to prevent problems with
    halo finding:

        IGNORE_PARTICLE_IDS = 1

    Note that in this case, merger trees are disabled.

    As mentioned above, Rockstar requires one master server accessible by
    all reading/analyis tasks.  The server does not require access to any
    data files, but it must be of the same architecture as the reading/
    analysis tasks.  In order to contact the master server, the reading/
    analysis tasks need to know its address, specified by the following two
    options in the config file:

    	PARALLEL_IO_SERVER_ADDRESS = <server address> #default: auto
        PARALLEL_IO_SERVER_PORT = <server port> #default: auto

    The server port should be in the range 1025-65000, and it should be
    unblocked by the server firewall.  Alternately, you may specify
    `"auto"` for both the server address and the server port.  In this case,
    once you launch the server process, it will write to a file called
    `"auto-rockstar.cfg"` in the launch directory (or the directory that
    OUTBASE is set to).  This file will contain updated settings with
    the machine name and port for the server, which you can then use
    to launch the reading/analysis tasks.  To force Rockstar to use a
    specified interface (useful for machines on multiple networks),
    you can use:

    	PARALLEL_IO_SERVER_INTERFACE = <interface name> # ib0, for example

    Since analysis tasks have to acquire particle data from reading tasks,
    the analysis tasks need to be contactable by the reading tasks.  You
    should choose a port number for the analysis tasks which is distinct
    from the server port above:

        PARALLEL_IO_WRITER_PORT = <port from 1025-65000>

    If you have a firewall, you will have to open this port for access
    **as well as** the `N` ports above this port, where `N` is the number of
    analysis tasks you are running per machine.

    The following options determine details of the halo finding.  To tell
    Rockstar to calculate halo radii as well as Vmax, Rvmax, and other
    halo properties from the **unbound** particles, set the following option
    to 0:

        BOUND_PROPS = <0 or 1> #default 1

    To change the default FOF refinement fraction (see the Rockstar paper),
    change:

        FOF_FRACTION = <FOF refinement fraction> #default 0.7

    To change the default 3D FOF linking length, set:

        FOF_LINKING_LENGTH = <FOF linking length> #default 0.28

    To set the force resolution (or equivalently, the softening length),
    set:

        FORCE_RES = <force resolution> #default: 0.0012, in comoving Mpc/h

    To change the minimum (internal) number of particles considered to be
    a halo seed, change:

        MIN_HALO_PARTICLES = <minimum number of halo particles> #default: 10

    See also MIN_HALO_OUTPUT_SIZE, above.

    To run postprocessing scripts after each snapshot finishes, you can use
    the `RUN_ON_SUCCESS` config option:

    	RUN_ON_SUCCESS = "/path/to/myscript"

    The script will be executed with two arguments: the snapshot number
    followed by the snapshot name (only different if specified via the
    `SNAPSHOT_NAMES` file).

    To specify your own load-balancing distribution, you can use the
    `LOAD_BALANCE_SCRIPT` option.  There is an example of such a script in
    the scripts directory; to use, set the `LOAD_BALANCE_SCRIPT` option to
    `"perl /path/to/rockstar/scripts/sample_loadbalance.pl"`.  The script is
    given the number of writer tasks, the recommended dimensional divisions,
    the box size, the scale factor, as well as the IP addresses and ports
    of all the writer tasks.  The script is expected to reoutput the
    information for each writer task along with an additional set of six
    numbers specifying the boundary region `(min_x, min_y, min_z)` to
    `(max_x, max_y, max_z)`.  The boundary region **must** be contained within
    the box size in each dimension; the boundary regions are also assumed to
    be overlapping (it is up to your script to check this).

9.  ### Full Example Script

    For a full example configuration file, used for running on the Bolshoi
    simulation, see `scripts/pleiades.cfg` or `scripts/ranger.cfg`.

    For the example PBS submission script used to run the halo finding on
    Pleiades, see scripts/pleiades.pbs; for Ranger, see scripts/ranger.pbs.

10. ### Example source code for loading particle outputs

    For additional useful source code to load in both the BGC2 and
    `FULL_PARTICLE` halo/particle formats, check the `examples/` subdirectory.