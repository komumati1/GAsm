class GAsm:
    """
    Genetic Assembly Machine (GAsm)

    Main engine for genetic programming.
    Provides control over population, mutation, crossover, growth,
    fitness evaluation, and evolution history.

    ---------------------------------------------------------------------
    Attributes
    ---------------------------------------------------------------------

    populationSize : int
        Number of individuals in the population.

    individualMaxSize : int
        Maximum allowed number of instructions in an individual.

    mutationProbability : float
        Probability of mutation during evolution.

    crossoverProbability : float
        Probability of crossover during evolution.

    maxGenerations : int
        Maximum number of generations to evolve.

    goalFitness : float
        Desired fitness target at which evolution stops.

    minimize : bool
        If True → lower fitness is better (minimization mode).
        If False → higher fitness is better.

    registerLength : int
        Number of registers available during program execution.

    maxProcessTime : int
        Maximum number of instructions executed before halting.

    best : Individual (read-only)
        Returns the best individual discovered so far.

    hist : Hist (read-only)
        Full history of evolution.
        Allows indexing like `hist[i]` to obtain an Entry.

    outputFolder : str
        Folder where logs, checkpoints, and outputs are stored.

    nanPenalty : float
        Fitness penalty applied if the program outputs NaN.

    useCompile : bool
        Enables or disables JIT compilation (if supported).

    checkpointInterval : int
        Save a checkpoint every N generations (0 = disabled).

    ---------------------------------------------------------------------
    Methods
    ---------------------------------------------------------------------
    """

    # ------------------------------------------------------------------
    # Core execution
    # ------------------------------------------------------------------

    def run(self, inputs: list[int]) -> list[int]:
        """Execute the current program on one input vector."""

    def runAll(self, inputs: list[list[int]]) -> list[list[int]]:
        """Execute the program on all inputs."""

    def setProgram(self, program: list[str]) -> None:
        """Replace the current program with one supplied by the user."""

    # ------------------------------------------------------------------
    # Evolution
    # ------------------------------------------------------------------

    def evolve(self, inputs: list[list[int]], outputs: list[list[int]]) -> None:
        """
        Run full evolutionary process.

        Parameters
        ----------
        inputs : list[list[int]]
            Training inputs.
        outputs : list[list[int]]
            Expected outputs.

        Evolution stops when:
            - goalFitness is reached OR
            - maxGenerations is exceeded.
        """

    # ------------------------------------------------------------------
    # Serialization
    # ------------------------------------------------------------------

    def save2File(self, path: str) -> None:
        """Save engine state (population + config + history) to a JSON file."""

    @staticmethod
    def fromJson(path: str) -> "GAsm":
        """Load engine state from a JSON file and return a new GAsm instance."""

    # ------------------------------------------------------------------
    # Selection
    # ------------------------------------------------------------------

    def setSelection(self, mode: str, param: int | float | None = None) -> None:
        """
        Set selection method.

        Parameters
        ----------
        mode : Literal["Tournament", "Truncation", "Boltzman", "Rank", "Roulette"]
            Selection strategy.

        param :
            Additional parameter depending on mode:
             - Tournament: tournament size (int)
             - Truncation: top-k percentage or count
             - Boltzman: temperature parameter
             - Rank: unused or tuning parameter
             - Roulette: no parameter required
        """

    # ------------------------------------------------------------------
    # Grow policy
    # ------------------------------------------------------------------

    def setGrow(self, mode: str, param: int | None = None) -> None:
        """
        Set growth mode for generating trees.

        Parameters
        ----------
        mode : Literal["Size", "Full", "Tree"]
            Grow strategy.

        param :
            Size limit or depth depending on mode.
        """

    # ------------------------------------------------------------------
    # Mutation & Crossover
    # ------------------------------------------------------------------

    def setMutation(self, mode: str) -> None:
        """
        Set mutation type.

        Parameters
        ----------
        mode : Literal["Hard", "Soft"]
        """

    def setCrossover(self, mode: str) -> None:
        """
        Set crossover method.

        Parameters
        ----------
        mode : Literal["OnePoint", "TwoPoint", "TwoPointSize", "Uniform"]
        """

    # ------------------------------------------------------------------
    # Generators (CNG & RNG)
    # ------------------------------------------------------------------

    def set_cng(self, type: str, start: float) -> None:
        """
        Set Custom Number Generator (CNG).

        Parameters
        ----------
        type : Literal["increment", "constant"]
        start : float
        """

    def set_rng(self, type: str, start: float) -> None:
        """
        Set Random Number Generator (RNG).

        Parameters
        ----------
        type : Literal["increment", "constant"]
        start : float
        """


class Hist:
    """
    Evolution history container (read-only).

    Behaves like a Python sequence:
        len(hist), hist[i], iteration, slicing

    Each entry is an `Entry` describing one generation.

    Methods
    -------
    __len__()
        Number of entries.
    __getitem__(i)
        Get the Entry at index i.
    """

    def __len__(self) -> int:
        """Return number of generations in the history."""
        ...

    def __getitem__(self, idx: int) -> "Entry":
        """
        Get history entry by index.

        Parameters
        ----------
        idx : int
            Index of the generation.

        Returns
        -------
        Entry
            Statistics and best individual of that generation.
        """
        ...


class Entry:
    """
    Statistics for a single generation.

    Attributes
    ----------
    generation : int
        Generation number.

    bestFitness : float
        Fitness of the best individual.

    avgFitness : float
        Average population fitness.

    avgSize : float
        Average program size.

    bestIndividual : Individual
        Best Individual of the generation.
    """

    @property
    def generation(self) -> int:
        """Generation index."""
        ...

    @property
    def bestFitness(self) -> float:
        """Fitness of the best program."""
        ...

    @property
    def avgFitness(self) -> float:
        """Average fitness of the population."""
        ...

    @property
    def avgSize(self) -> float:
        """Average program size."""
        ...

    @property
    def best(self) -> "Individual":
        """Best program (Individual) from this generation."""
        ...


class Individual:
    """
    Evolvable program represented as bytecode.

    Methods
    -------
    run(inputs)
        Executes the program on given input registers.
    toString()
        Return a human-readable text version of the bytecode.

    Attributes
    ----------
    maxProcessTime : int
        Maximum number of instructions.

    registerLength : int
        Number of registers used.

    cng : Callable[[], float]
        Constant number generator (CNG) used for growth/mutation.

    rng : Callable[[], float]
        Random number generator (RNG) used for growth/mutation.

    useCompile : bool
        Whether compiled execution is used internally.
    """

    def run(self, inputs: list[float]) -> int:
        """
        Execute the program.

        Parameters
        ----------
        inputs : list[float]
            Initial register values.

        Returns
        -------
        int
            Execution result (instruction count or program output, depending on implementation).
        """
        ...

    def toString(self) -> str:
        """
        Return the textual representation of the bytecode.

        Returns
        -------
        str
            Pretty-printed assembly-like representation.
        """
        ...


