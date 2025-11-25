// SPDX-License-Identifier: MIT
pragma solidity ^0.8.30;

contract Target {
    mapping(address => uint256) public balances;
    mapping(address => uint256) public rewards;
    mapping(address => uint256) public lastAction;
    uint256 public totalDeposits;
    bool private locked;

    constructor() payable {
        require(msg.value == 100 ether, "Deploy with 100 ETH seed");
        totalDeposits += msg.value;
        balances[msg.sender] = msg.value;
    }

    modifier noReentrant() {
        require(!locked, "Reentrant call");
        locked = true;
        _;
        locked = false;
    }

    function deposit() external payable {
        balances[msg.sender] += msg.value;
        totalDeposits += msg.value;
    }

    function withdraw(uint256 amount) external noReentrant {
        require(balances[msg.sender] >= amount, "Not enough balance");
        balances[msg.sender] -= amount;
        totalDeposits -= amount;
        payable(msg.sender).transfer(amount);
    }

    function giveMeAReward(address user) external {
        notSoFast();
        uint256 reward = 1 ether;
        (bool ok, ) = user.call{value: reward}("");
        require(ok, "send failed");
        lastAction[msg.sender] = block.timestamp;
    }

    function notSoFast() internal view {
        require(
            block.timestamp >= lastAction[msg.sender] + 5 minutes,
            "Wait at least 5 minutes"
        );
    }

    function getMyBalance() external view returns (uint256) {
        return balances[msg.sender];
    }
}
