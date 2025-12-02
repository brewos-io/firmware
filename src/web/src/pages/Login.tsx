import { useEffect } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { GoogleLogin, CredentialResponse } from '@react-oauth/google';
import { Card } from '@/components/Card';
import { Logo } from '@/components/Logo';
import { useAuth } from '@/lib/auth';
import { isGoogleAuthConfigured } from '@/lib/auth';
import { enableDemoMode } from '@/lib/demo-mode';
import { useThemeStore } from '@/lib/themeStore';
import { AlertCircle, Play } from 'lucide-react';

export function Login() {
  const navigate = useNavigate();
  const location = useLocation();
  const { user, handleGoogleLogin } = useAuth();
  const { theme } = useThemeStore();
  const isDark = theme.isDark;
  
  // Get error from navigation state (e.g., from failed auth)
  const error = (location.state as { error?: string })?.error;

  useEffect(() => {
    if (user) {
      navigate('/devices');
    }
  }, [user, navigate]);

  if (!isGoogleAuthConfigured) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-coffee-800 to-coffee-900 flex items-center justify-center p-4">
        <Card className="w-full max-w-md text-center">
          <div className="flex justify-center mb-4">
            <Logo size="lg" forceDark />
          </div>
          <p className="text-coffee-500 mb-6">
            Cloud features are not configured. Connect directly to your device at{' '}
            <a href="http://brewos.local" className="text-accent hover:underline">
              brewos.local
            </a>
          </p>
          <div className={`border-t pt-6 mt-6 ${isDark ? 'border-coffee-700' : 'border-cream-300'}`}>
            <button
              onClick={() => {
                enableDemoMode();
                navigate('/');
                window.location.reload(); // Reload to initialize demo mode
              }}
              className={`w-full flex items-center justify-center gap-2 px-4 py-3 
                         font-medium rounded-lg transition-all group
                         ${isDark 
                           ? 'bg-gradient-to-r from-violet-500/20 to-purple-500/20 hover:from-violet-500/30 hover:to-purple-500/30 border border-violet-400/30 hover:border-violet-400/50 text-violet-300' 
                           : 'bg-gradient-to-r from-violet-500/10 to-purple-500/10 hover:from-violet-500/20 hover:to-purple-500/20 border border-violet-500/30 hover:border-violet-500/50 text-violet-700'
                         }`}
            >
              <Play className="w-4 h-4 group-hover:scale-110 transition-transform" />
              Try Demo
            </button>
            <p className="text-center text-xs text-theme-muted mt-2">
              Explore the app with simulated machine data
            </p>
          </div>
        </Card>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-coffee-800 to-coffee-900 flex items-center justify-center p-4">
      <Card className="w-full max-w-md">
        <div className="text-center mb-8">
          <div className="flex justify-center mb-4">
            <Logo size="lg" />
          </div>
          <h1 className="text-2xl font-bold text-theme">Welcome to BrewOS</h1>
          <p className="text-theme-muted mt-2">
            Sign in to manage your espresso machines from anywhere
          </p>
        </div>

        {error && (
          <div className={`mb-4 p-3 rounded-lg flex items-center gap-2 ${
            isDark 
              ? 'bg-red-900/30 border border-red-400/30 text-red-300' 
              : 'bg-red-50 border border-red-200 text-red-700'
          }`}>
            <AlertCircle className="w-4 h-4 shrink-0" />
            <span className="text-sm">{error}</span>
          </div>
        )}

        <div className="space-y-4">
          <div className="flex justify-center">
            <GoogleLogin
              onSuccess={(response: CredentialResponse) => {
                if (response.credential) {
                  handleGoogleLogin(response.credential);
                }
              }}
              onError={() => {
                console.error('Google login failed');
              }}
              theme="outline"
              size="large"
              width="300"
              text="continue_with"
            />
          </div>

          <div className="relative">
            <div className="absolute inset-0 flex items-center">
              <div className={`w-full border-t ${isDark ? 'border-coffee-700' : 'border-cream-300'}`}></div>
            </div>
            <div className="relative flex justify-center text-sm">
              <span className={`px-4 ${isDark ? 'bg-[var(--card-bg)] text-coffee-500' : 'bg-white text-coffee-400'}`}>or</span>
            </div>
          </div>

          <p className="text-center text-sm text-theme-muted">
            Connect locally at{' '}
            <a href="http://brewos.local" className="text-accent hover:underline">
              brewos.local
            </a>
          </p>

          <div className="relative">
            <div className="absolute inset-0 flex items-center">
              <div className={`w-full border-t ${isDark ? 'border-coffee-700' : 'border-cream-300'}`}></div>
            </div>
            <div className="relative flex justify-center text-sm">
              <span className={`px-4 ${isDark ? 'bg-[var(--card-bg)] text-coffee-500' : 'bg-white text-coffee-400'}`}>or</span>
            </div>
          </div>

          <button
            onClick={() => {
              enableDemoMode();
              navigate('/');
              window.location.reload(); // Reload to initialize demo mode
            }}
            className={`w-full flex items-center justify-center gap-2 px-4 py-3 
                       font-medium rounded-lg transition-all group
                       ${isDark 
                         ? 'bg-gradient-to-r from-violet-500/20 to-purple-500/20 hover:from-violet-500/30 hover:to-purple-500/30 border border-violet-400/30 hover:border-violet-400/50 text-violet-300' 
                         : 'bg-gradient-to-r from-violet-500/10 to-purple-500/10 hover:from-violet-500/20 hover:to-purple-500/20 border border-violet-500/30 hover:border-violet-500/50 text-violet-700'
                       }`}
          >
            <Play className="w-4 h-4 group-hover:scale-110 transition-transform" />
            Try Demo
          </button>
          <p className="text-center text-xs text-theme-muted">
            Explore the app with simulated machine data
          </p>
        </div>
      </Card>
    </div>
  );
}
