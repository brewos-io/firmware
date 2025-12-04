import { addons } from "@storybook/manager-api";
import { create } from "@storybook/theming/create";

// Custom Storybook theme that matches BrewOS branding
const brewOSTheme = create({
  base: "dark",
  
  // Brand
  brandTitle: "BrewOS Design System",
  brandUrl: "https://brewos.io",
  brandTarget: "_blank",
  
  // Colors
  colorPrimary: "#c4703c",
  colorSecondary: "#d4a45c",
  
  // UI
  appBg: "#1a1612",
  appContentBg: "#241c18",
  appBorderColor: "#352818",
  appBorderRadius: 8,
  
  // Typography
  fontBase: '"Plus Jakarta Sans", system-ui, sans-serif',
  fontCode: '"JetBrains Mono", monospace',
  
  // Text colors
  textColor: "#f5e8d8",
  textInverseColor: "#1a1612",
  textMutedColor: "#a89878",
  
  // Toolbar default and active colors
  barTextColor: "#d4c4a8",
  barSelectedColor: "#d4a45c",
  barBg: "#181410",
  
  // Form colors
  inputBg: "#241e18",
  inputBorder: "#352818",
  inputTextColor: "#f5e8d8",
  inputBorderRadius: 8,
});

addons.setConfig({
  theme: brewOSTheme,
  sidebar: {
    showRoots: true,
  },
});

