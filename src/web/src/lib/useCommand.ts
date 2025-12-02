import { useCallback } from 'react';
import { useStore } from './store';
import { getConnection } from './connection';
import { useToast } from '@/components/Toast';

interface UseCommandOptions {
  /** Toast message to show on success */
  successMessage?: string;
  /** Toast message to show on error */
  errorMessage?: string;
  /** Skip connection check (for actions that work offline) */
  skipConnectionCheck?: boolean;
}

/**
 * Hook for sending commands with built-in connection checking and error handling
 */
export function useCommand() {
  const connectionState = useStore((s) => s.connectionState);
  const { success, error } = useToast();
  
  const isConnected = connectionState === 'connected';
  
  /**
   * Send a command to the ESP32 with error handling
   */
  const sendCommand = useCallback((
    command: string, 
    payload?: Record<string, unknown>,
    options: UseCommandOptions = {}
  ): boolean => {
    const { 
      successMessage, 
      errorMessage = 'Action failed. Please try again.',
      skipConnectionCheck = false 
    } = options;
    
    // Check connection
    if (!skipConnectionCheck && !isConnected) {
      error('Not connected to machine');
      return false;
    }
    
    try {
      const connection = getConnection();
      if (!connection) {
        error('No connection available');
        return false;
      }
      
      connection.sendCommand(command, payload);
      
      if (successMessage) {
        success(successMessage);
      }
      
      return true;
    } catch (err) {
      console.error(`Command ${command} failed:`, err);
      error(errorMessage);
      return false;
    }
  }, [isConnected, success, error]);
  
  /**
   * Send a command that requires user confirmation
   */
  const sendCommandWithConfirm = useCallback((
    command: string,
    confirmMessage: string,
    payload?: Record<string, unknown>,
    options: UseCommandOptions = {}
  ): boolean => {
    if (!confirm(confirmMessage)) {
      return false;
    }
    return sendCommand(command, payload, options);
  }, [sendCommand]);
  
  return {
    sendCommand,
    sendCommandWithConfirm,
    isConnected,
  };
}

