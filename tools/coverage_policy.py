#!/usr/bin/env python3
"""Resolve and validate mbo's coverage policy single source of truth."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


METRICS = ("lines", "functions", "branches")
RATINGS = ("low", "medium", "high")


@dataclass(frozen=True)
class MetricPolicy:
    minimum: float
    target: float
    enforce: str


def _metric_values(value: Any, default: dict[str, Any] | None = None) -> dict[str, Any]:
    if value is None:
        return dict(default or {})
    if isinstance(value, str):
        return {metric: value for metric in METRICS}
    if not isinstance(value, dict):
        raise ValueError("coverage policy metric values must be objects or a rating name")
    unknown = set(value) - set(METRICS)
    if unknown:
        raise ValueError(f"unknown coverage metrics: {', '.join(sorted(unknown))}")
    return {**(default or {}), **value}


def resolve(scope: dict[str, Any], parent: dict[str, MetricPolicy] | None = None) -> dict[str, MetricPolicy]:
    """Returns one scope's effective per-metric boundaries and enforcement."""
    parent = parent or {}
    minimums = _metric_values(scope.get("minimum"))
    targets = _metric_values(scope.get("target"))
    enforcement = _metric_values(scope.get("enforce"))
    result: dict[str, MetricPolicy] = {}
    for metric in METRICS:
        inherited = parent.get(metric)
        minimum = minimums.get(metric, inherited.minimum if inherited else None)
        inherited_target = inherited.target if inherited else minimum
        target = targets.get(metric, max(minimum, inherited_target))
        enforce = enforcement.get(metric, inherited.enforce if inherited else "medium")
        if minimum is None or target is None:
            raise ValueError(f"coverage policy is missing {metric} boundaries")
        if not isinstance(minimum, (int, float)) or not isinstance(target, (int, float)):
            raise ValueError(f"{metric} coverage boundaries must be numbers")
        if not 0 <= minimum <= target <= 100:
            label = {"lines": "line", "functions": "function", "branches": "branch"}[metric]
            raise ValueError(
                f"{label} coverage thresholds must satisfy 0 <= minimum <= target <= 100: "
                f"{minimum}, {target}"
            )
        if enforce not in ("medium", "high"):
            raise ValueError(f"{metric} coverage enforcement must be medium or high: {enforce!r}")
        result[metric] = MetricPolicy(float(minimum), float(target), enforce)
    return result


def overall(policy: dict[str, Any]) -> dict[str, MetricPolicy]:
    if "bands" in policy:
        raise ValueError("coverage presentation bands are not separate from minimum and target")
    return resolve(policy)


def is_weaker(candidate: dict[str, MetricPolicy], parent: dict[str, MetricPolicy]) -> bool:
    """Whether an override lowers a boundary or relaxes enforcement."""
    for metric in METRICS:
        value = candidate[metric]
        inherited = parent[metric]
        if value.minimum < inherited.minimum or value.target < inherited.target:
            return True
        if RATINGS.index(value.enforce) < RATINGS.index(inherited.enforce):
            return True
    return False


def validate_override(scope: dict[str, Any], parent: dict[str, MetricPolicy], name: str) -> dict[str, MetricPolicy]:
    result = resolve(scope, parent)
    if is_weaker(result, parent) and not str(scope.get("reason", "")).strip():
        raise ValueError(f"weaker coverage override for {name} requires a reason")
    return result


def policies(policy: dict[str, Any]) -> dict[str, dict[str, MetricPolicy]]:
    """Returns effective policies for the overall row and every category row."""
    inherited = overall(policy)
    result = {"overall": inherited}
    for name, category in policy.get("categories", {}).items():
        result[name] = validate_override(category, inherited, name)
    if "patch" in policy:
        resolve(policy["patch"], inherited)
    return result


def baseline_tolerances(policy: dict[str, Any]) -> dict[str, float]:
    """Returns the allowed percentage-point regression for each metric."""
    baseline = policy.get("baseline")
    if not isinstance(baseline, dict):
        raise ValueError("coverage policy is missing the baseline configuration")
    maximum_drop = baseline.get("maximum_drop")
    if not isinstance(maximum_drop, dict):
        raise ValueError("coverage baseline maximum_drop must be an object")
    unknown = set(maximum_drop) - set(METRICS)
    if unknown:
        raise ValueError(f"unknown coverage metrics: {', '.join(sorted(unknown))}")
    missing = set(METRICS) - set(maximum_drop)
    if missing:
        raise ValueError(f"coverage baseline is missing metrics: {', '.join(sorted(missing))}")
    result = {}
    for metric in METRICS:
        value = maximum_drop[metric]
        if not isinstance(value, (int, float)) or isinstance(value, bool) or not 0 <= value <= 100:
            raise ValueError(f"{metric} coverage baseline maximum drop must be between 0 and 100")
        result[metric] = float(value)
    return result


def rating(percent: float | None, policy: MetricPolicy) -> str:
    if percent is None or percent < policy.minimum:
        return "low"
    if percent < policy.target:
        return "medium"
    return "high"


def passes(percent: float | None, policy: MetricPolicy) -> bool:
    return RATINGS.index(rating(percent, policy)) >= RATINGS.index(policy.enforce)


def serializable(values: dict[str, MetricPolicy]) -> dict[str, dict[str, float | str]]:
    return {
        "minimum": {metric: value.minimum for metric, value in values.items()},
        "target": {metric: value.target for metric, value in values.items()},
        "enforce": {metric: value.enforce for metric, value in values.items()},
    }
