export type MembershipTier = 'free' | 'lifetime';

export type MockUser = {
  id: string;
  email: string;
  membership: MembershipTier;
};

export async function getCurrentUser(): Promise<MockUser> {
  await new Promise((resolve) => setTimeout(resolve, 120));

  // Mocked for now: switch to "lifetime" to preview unlocked premium downloads.
  return {
    id: 'user_demo_001',
    email: 'demo@vstgarage.local',
    membership: 'free'
  };
}

export async function createLifetimeCheckoutSession(): Promise<{ checkoutUrl: string }> {
  await new Promise((resolve) => setTimeout(resolve, 120));

  return {
    checkoutUrl: '/mock/stripe/checkout?plan=lifetime'
  };
}
