import type { Config } from 'tailwindcss';

const config: Config = {
  content: ['./src/**/*.{js,ts,jsx,tsx,mdx}'],
  theme: {
    extend: {
      colors: {
        garage: {
          bg: '#0a0d14',
          card: '#111827',
          accent: '#38bdf8'
        }
      }
    }
  },
  plugins: []
};

export default config;
