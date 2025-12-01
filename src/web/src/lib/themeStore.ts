import { create } from 'zustand';
import { Theme, ThemeId, themes, applyTheme, getStoredTheme } from './themes';

interface ThemeState {
  theme: Theme;
  setTheme: (themeId: ThemeId) => void;
  initTheme: () => void;
}

export const useThemeStore = create<ThemeState>((set) => ({
  theme: themes.classic,
  
  setTheme: (themeId: ThemeId) => {
    const theme = themes[themeId];
    if (theme) {
      applyTheme(theme);
      set({ theme });
    }
  },
  
  initTheme: () => {
    const theme = getStoredTheme();
    applyTheme(theme);
    set({ theme });
  },
}));

