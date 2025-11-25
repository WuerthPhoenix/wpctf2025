from typing import Callable

from pyo3_crypto.pyo3_crypto import (
    clever_turing,
    diligent_tesla,
    elated_liskov,
    fervent_ramanujan,
    flamboyant_shannon,
    frosty_shannon,
    gifted_brahmagupta,
    interesting_agnesi,
    jovial_babbage,
    meticulous_einstein,
    pensive_engelbart,
    tender_dijkstra,
    thirsty_leakey,
    youthful_dhawan,
    zealous_visvesvaraya,
)

FUNCTIONS: list[Callable[[bytes], bytes]] = [
    fervent_ramanujan,
    diligent_tesla,
    elated_liskov,
    clever_turing,
    thirsty_leakey,
    interesting_agnesi,
    gifted_brahmagupta,
    frosty_shannon,
    pensive_engelbart,
    youthful_dhawan,
    fervent_ramanujan,
    zealous_visvesvaraya,
    flamboyant_shannon,
]


def main() -> None:
    print("OSCO Protocol, Admin Access ONLY, All rights reserved.")
    user_input = input("~> ").encode()

    ord: list[int] = meticulous_einstein(len(FUNCTIONS))

    user_input = fervent_ramanujan(user_input)
    user_input = diligent_tesla(user_input)
    user_input = elated_liskov(user_input)
    user_input = clever_turing(user_input)
    user_input = thirsty_leakey(user_input)
    user_input = interesting_agnesi(user_input)
    user_input = gifted_brahmagupta(user_input)
    for idx in ord:
        user_input = FUNCTIONS[idx](user_input)
    user_input = frosty_shannon(user_input)
    user_input = pensive_engelbart(user_input)
    user_input = youthful_dhawan(user_input)
    user_input = fervent_ramanujan(user_input)
    user_input = zealous_visvesvaraya(user_input)
    user_input = flamboyant_shannon(user_input)
    user_input = tender_dijkstra(user_input)

    print(jovial_babbage(user_input), end="")


if __name__ == "__main__":
    try:
        main()
    except BaseException:
        pass
