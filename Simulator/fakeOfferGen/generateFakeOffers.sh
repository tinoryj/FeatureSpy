#!/bin/bash
mkdir result
for ((bonus = 30; bonus < 61; bonus += 1)); do
    for ((salary = 204; salary < 810; salary += 6)); do
        python3 modifyFile.py offer-base result/ $salary $bonus
    done
done
