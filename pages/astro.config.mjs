import { defineConfig } from "astro/config";

export default defineConfig({
  site: "https://brewos.io",
  output: "static",
  build: {
    assets: "_assets",
  },
});
