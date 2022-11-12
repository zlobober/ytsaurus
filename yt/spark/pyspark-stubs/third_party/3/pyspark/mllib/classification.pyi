# Stubs for pyspark.mllib.classification (Python 3.5)
#

from typing import overload
from typing import Any, Optional, Union
from pyspark.context import SparkContext
from pyspark.rdd import RDD
from pyspark.mllib._typing import VectorLike
from pyspark.mllib.linalg import Vector
from pyspark.mllib.regression import LabeledPoint, LinearModel, StreamingLinearAlgorithm
from pyspark.mllib.util import Saveable, Loader
from pyspark.streaming.dstream import DStream
from numpy import float64, ndarray  # type: ignore

class LinearClassificationModel(LinearModel):
    def __init__(self, weights: Vector, intercept: float) -> None: ...
    def setThreshold(self, value: float) -> None: ...
    @property
    def threshold(self) -> Optional[float]: ...
    def clearThreshold(self) -> None: ...
    @overload
    def predict(self, test: VectorLike) -> Union[int, float, float64]: ...
    @overload
    def predict(self, test: RDD[VectorLike]) -> RDD[Union[int, float]]: ...

class LogisticRegressionModel(LinearClassificationModel):
    def __init__(
        self, weights: Vector, intercept: float, numFeatures: int, numClasses: int
    ) -> None: ...
    @property
    def numFeatures(self) -> int: ...
    @property
    def numClasses(self) -> int: ...
    @overload
    def predict(self, x: VectorLike) -> Union[int, float]: ...
    @overload
    def predict(self, x: RDD[VectorLike]) -> RDD[Union[int, float]]: ...
    def save(self, sc: SparkContext, path: str) -> None: ...
    @classmethod
    def load(cls, sc: SparkContext, path: str) -> LogisticRegressionModel: ...

class LogisticRegressionWithSGD:
    @classmethod
    def train(
        cls,
        data: RDD[LabeledPoint],
        iterations: int = ...,
        step: float = ...,
        miniBatchFraction: float = ...,
        initialWeights: Optional[VectorLike] = ...,
        regParam: float = ...,
        regType: str = ...,
        intercept: bool = ...,
        validateData: bool = ...,
        convergenceTol: float = ...,
    ) -> LogisticRegressionModel: ...

class LogisticRegressionWithLBFGS:
    @classmethod
    def train(
        cls,
        data: RDD[LabeledPoint],
        iterations: int = ...,
        initialWeights: Optional[VectorLike] = ...,
        regParam: float = ...,
        regType: str = ...,
        intercept: bool = ...,
        corrections: int = ...,
        tolerance: float = ...,
        validateData: bool = ...,
        numClasses: int = ...,
    ) -> LogisticRegressionModel: ...

class SVMModel(LinearClassificationModel):
    def __init__(self, weights: Vector, intercept: float) -> None: ...
    @overload
    def predict(self, x: VectorLike) -> float64: ...
    @overload
    def predict(self, x: RDD[VectorLike]) -> RDD[float64]: ...
    def save(self, sc: SparkContext, path: str) -> None: ...
    @classmethod
    def load(cls, sc: SparkContext, path: str) -> SVMModel: ...

class SVMWithSGD:
    @classmethod
    def train(
        cls,
        data: RDD[LabeledPoint],
        iterations: int = ...,
        step: float = ...,
        regParam: float = ...,
        miniBatchFraction: float = ...,
        initialWeights: Optional[VectorLike] = ...,
        regType: str = ...,
        intercept: bool = ...,
        validateData: bool = ...,
        convergenceTol: float = ...,
    ) -> SVMModel: ...

class NaiveBayesModel(Saveable, Loader[NaiveBayesModel]):
    labels: ndarray
    pi: ndarray
    theta: ndarray
    def __init__(self, labels, pi, theta) -> None: ...
    @overload
    def predict(self, x: VectorLike) -> float64: ...
    @overload
    def predict(self, x: RDD[VectorLike]) -> RDD[float64]: ...
    def save(self, sc: SparkContext, path: str) -> None: ...
    @classmethod
    def load(cls, sc: SparkContext, path: str) -> NaiveBayesModel: ...

class NaiveBayes:
    @classmethod
    def train(cls, data: RDD[VectorLike], lambda_: float = ...) -> NaiveBayesModel: ...

class StreamingLogisticRegressionWithSGD(StreamingLinearAlgorithm):
    stepSize: float
    numIterations: int
    regParam: float
    miniBatchFraction: float
    convergenceTol: float
    def __init__(
        self,
        stepSize: float = ...,
        numIterations: int = ...,
        miniBatchFraction: float = ...,
        regParam: float = ...,
        convergenceTol: float = ...,
    ) -> None: ...
    def setInitialWeights(
        self, initialWeights: VectorLike
    ) -> StreamingLogisticRegressionWithSGD: ...
    def trainOn(self, dstream: DStream[LabeledPoint]) -> None: ...