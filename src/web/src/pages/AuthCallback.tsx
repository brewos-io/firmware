import { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { Loading } from '@/components/Loading';

/**
 * Auth callback page - redirects to appropriate page
 * With Google OAuth, we don't need a callback page since 
 * Google uses popup/redirect and returns directly to the app
 */
export function AuthCallback() {
  const navigate = useNavigate();

  useEffect(() => {
    // Just redirect to devices or login
    navigate('/machines');
  }, [navigate]);

  return <Loading message="Redirecting..." />;
}
