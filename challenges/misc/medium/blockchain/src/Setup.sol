// SPDX-License-Identifier: MIT
pragma solidity 0.8.30;

import {Target} from "./Target.sol";

contract Setup {
    Target public immutable TARGET;

    constructor() payable {
        TARGET = new Target{value: 100 ether}();
    }

    function isSolved() public view returns (bool) {
        return address(TARGET).balance == 0;
    }
}
