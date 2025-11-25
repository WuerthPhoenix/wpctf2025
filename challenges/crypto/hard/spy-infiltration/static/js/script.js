        document.getElementById("save-btn").addEventListener("click", async (event) => {
            event.preventDefault();

            const comment = document.getElementById("comment").value;
            const baudRate = document.getElementById("baud_rate").value;
            const timezone = document.getElementById("timezone").value;
            const mode = document.getElementById("mode").value;

            try {
                const response = await fetch("/config", {
                    method: "POST",
                    headers: {
                        "Content-Type": "application/json",
                    },
                    body: JSON.stringify({
                        comment: comment,
                        baud_rate: baudRate,
                        timezone: timezone,
                        mode: mode,
                    }),
                });

                const result = await response.json();
                const statusMessage = document.getElementById("error-message");

                if (response.ok) {
                    statusMessage.classList.remove("alert-danger", "hidden");
                    statusMessage.classList.add("alert-success");
                    statusMessage.textContent = "Configuration saved successfully!";
                } else {
                    statusMessage.classList.remove("alert-success", "hidden");
                    statusMessage.classList.add("alert-danger");
                    statusMessage.textContent = result.error || "Failed to save configuration.";
                }
            } catch (error) {
                console.error("Error saving configuration:", error);
            }
        });

        document.getElementById("download-btn").addEventListener("click", async (event) => {
            event.preventDefault();

            try {
                const response = await fetch("config/download", {
                    method: "GET",
                });

                const result = await response.json();
                const statusMessage = document.getElementById("error-message");

                if (response.ok) {
                    const blob = new Blob([result.encrypted_config], { type: "text/plain" });
                    const link = document.createElement("a");
                    link.href = URL.createObjectURL(blob);
                    link.download = "config.txt";
                    link.click();

                    statusMessage.classList.remove("alert-danger", "hidden");
                    statusMessage.classList.add("alert-success");
                    statusMessage.textContent = "Configuration downloaded successfully!";
                } else {
                    statusMessage.classList.remove("alert-success", "hidden");
                    statusMessage.classList.add("alert-danger");
                    statusMessage.textContent = result.error || "Failed to download configuration.";
                }
            } catch (error) {
                console.error("Error downloading configuration:", error);
            }
        });