from typing import Literal, Optional


class Individual:
    """
    Genetic-program individual.

    Returned by:
        GAsm.best
        Entry.bestIndividual

    Methods
    -------
    run(inputs: list[int]) -> list[int]
        Execute on one input vector.

    toString() -> str
        Return textual form of bytecode.
    """

    def run(self, inputs: list[int]) -> list[int]: ...
    def toString(self) -> str: ...


class Entry:
    """
    One generation in evolutionary history.

    Attributes
    ----------
    generation : int
    bestFitness : float
    averageFitness : float
    bestIndividual : Individual
    timestamp : float

    Access via:
        hist[i]
    """

    generation: int
    bestFitness: float
    averageFitness: float
    bestIndividual: Individual
    timestamp: float


class Hist:
    """
    Read-only evolutionary history.

    Behaves like a list of Entry.
    """

    def __len__(self) -> int: ...
    def __getitem__(self, idx: int) -> Entry: ...


class GAsm:
    """
    Genetic Assembly Machine.

    Main engine for evolutionary computation.
    """

    # ------------------------------------------------------------------
    # Attributes
    # ------------------------------------------------------------------
    populationSize: int
    individualMaxSize: int
    mutationProbability: float
    crossoverProbability: float
    maxGenerations: int
    goalFitness: float
    minimize: bool
    registerLength: int
    maxProcessTime: int

    best: Individual          # read-only
    hist: Hist                # read-only

    outputFolder: str
    nanPenalty: float
    useCompile: bool
    checkpointInterval: int

    # ------------------------------------------------------------------
    # Core Execution
    # ------------------------------------------------------------------
    def run(self, inputs: list[int]) -> list[int]: ...
    def runAll(self, inputs: list[list[int]]) -> list[list[int]]: ...
    def setProgram(self, program: list[str]) -> None: ...

    # ------------------------------------------------------------------
    # Evolution
    # ------------------------------------------------------------------
    def evolve(self, inputs: list[list[int]], outputs: list[list[int]]) -> None:
        """
        Run full evolutionary training.
        """

    # ------------------------------------------------------------------
    # Serialization
    # ------------------------------------------------------------------
    def save2File(self, path: str) -> None: ...

    @staticmethod
    def fromJson(path: str) -> "GAsm": ...

    # ------------------------------------------------------------------
    # Configuration: Selection
    # ------------------------------------------------------------------
    def setSelection(
            self,
            mode: Literal["Tournament", "Truncation", "Boltzman", "Rank", "Roulette"],
            param: Optional[float | int] = None
    ) -> None:
        """
        Set selection strategy.
        """

    # ------------------------------------------------------------------
    # Configuration: Grow
    # ------------------------------------------------------------------
    def setGrow(
            self,
            mode: Literal["Size", "Full", "Tree"],
            param: Optional[int] = None
    ) -> None:
        """
        Set grow strategy for initial individuals.
        """

    # ------------------------------------------------------------------
    # Configuration: Mutation & Crossover
    # ------------------------------------------------------------------
    def setMutation(
            self,
            mode: Literal["Hard", "Soft"]
    ) -> None: ...

    def setCrossover(
            self,
            mode: Literal["OnePoint", "TwoPoint", "TwoPointSize", "Uniform"]
    ) -> None: ...

    # ------------------------------------------------------------------
    # Custom Generators
    # ------------------------------------------------------------------
    def set_cng(
            self,
            type: Literal["increment", "constant"],
            start: float
    ) -> None: ...

    def set_rng(
            self,
            type: Literal["random", "constant"],
            start: float
    ) -> None: ...
